#!/bin/env elixir

Mix.install([
  {:circuits_uart, "~> 1.5"}
])

defmodule Midi do
  @state_none 0
  @state_get_note 10
  @state_get_velocity 20
  @state_ready 100

  @type_none 0
  @type_note_on 1
  @type_note_off 2

  # this message is sent periodically from the keytar
  def process(<<254>>,_,_) do
    %{state: @state_none, type: @type_none}
  end
  
  # 0x9x = MIDI on, x = channel, -> get note, followed by velocity
  def process(<<9::4,channel::4>>,_,prior) do
    Map.merge(prior, %{state: @state_get_note, type: @type_note_on, channel: channel})
  end
  
  # 0x9x = MIDI off, x = channel, -> get note
  def process(<<8::4,channel::4>>,_,prior) do
    Map.merge(prior, %{state: @state_get_note, type: @type_note_off, channel: channel})
  end
  
  # add midi note to map -> get velocity
  def process(byte,@state_get_note,prior) do
    <<note>> = byte
    Map.merge(prior, %{state: @state_get_velocity, note: note})
  end
  
  # add velocity to map -> ready to be used
  def process(byte,@state_get_velocity,prior) do
    <<velocity>> = byte
    Map.merge(prior, %{state: @state_ready, velocity: velocity})
  end
  
  def process(_,_,_) do
    # this is a state we don't understand, ignore it and reset
    # IO.puts "i don't know what to do"
    %{state: @state_none, type: @type_none}
  end

  def action(@state_ready, s, processor, arg) do
    processor.(s, arg)
    s
  end

  def action(_,s,_,_) do
    s
  end

  def loop(byte, current, processor, arg) do
    s = process(byte, current.state, current)
    action(s.state, s, processor, arg)
  end

  def open(port) do
    {:ok, pid} = Circuits.UART.start_link
    Circuits.UART.open(pid, port, speed: 31250, active: false)
    pid
  end

  def run(state, pid, processor, arg) do
    {:ok, raw} = Circuits.UART.read(pid, 1000)
    String.graphemes(raw)
      |> Enum.reduce(%{state: state}, fn n, state ->
           loop(n, state, processor, arg)
         end)
    run(state, pid, processor, arg)
  end
end

# u = Circuits.UART.enumerate

defmodule Custom do
  def open(addr, port) do
    {:ok, socket} = :gen_udp.open(0)
    outlet = %{socket: socket, addr: addr, port: port}
    wire("[34", outlet)
    outlet
  end
  def wire(msg, outlet) do
    :gen_udp.send(outlet.socket, outlet.addr, outlet.port, msg)
  end
  def processor(s, outlet) do
    case s.type do
      # Midi.note_on ->
      1 ->
        v = 40
        x = s.note
        y = s.note - 12
        z = s.note + 0.1
        IO.puts " ON #{s.channel} #{s.note} #{s.velocity}"
        wire("v#{v}n#{x}l5 v#{v+1}n#{y}l5 v#{v+2}n#{z}l5", outlet)
      # Midi.note_off ->
      2 ->
        v = 40
        IO.puts "OFF #{s.channel} #{s.note} #{s.velocity}"
        wire("v#{v}l0 v#{v+1}l0 v#{v+2}l0", outlet)
    end
  end
end

args = System.argv()
port = Enum.at(args, 0)
pid = Midi.open(port)
outlet = Custom.open({0,0,0,0}, 60440)
Midi.run(0, pid, &Custom.processor/2, outlet)
