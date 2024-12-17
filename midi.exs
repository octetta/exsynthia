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
  def consume(<<254>>,prior) do
    Map.merge(prior, %{state: @state_none, type: @type_none})
  end
  
  # 0x9x = MIDI on, x = channel, -> get note, followed by velocity
  def consume(<<9::4,channel::4>>,prior) do
    Map.merge(prior, %{state: @state_get_note, type: @type_note_on, channel: channel})
  end
  
  # 0x9x = MIDI off, x = channel, -> get note
  def consume(<<8::4,channel::4>>,prior) do
    Map.merge(prior, %{state: @state_get_note, type: @type_note_off, channel: channel})
  end
  
  # add midi note to map -> get velocity
  def consume(byte,%{state: @state_get_note}=prior) do
    <<note>> = byte
    Map.merge(prior, %{state: @state_get_velocity, note: note})
  end
  
  # add velocity to map -> ready to be used
  def consume(byte,%{state: @state_get_velocity}=prior) do
    <<velocity>> = byte
    Map.merge(prior, %{state: @state_ready, velocity: velocity})
  end
  
  def consume(_,_prior) do
    # this is a state we don't understand, ignore it and reset
    # IO.puts "i don't know what to do"
    %{state: @state_none, type: @type_none}
  end

  def action(%{state: @state_ready}=s, consumer, arg) do
    consumer.(s, arg)
    s
  end

  def action(s,_,_) do
    s
  end

  def open(port) do
    {:ok, pid} = Circuits.UART.start_link
    Circuits.UART.open(pid, port, speed: 31250, active: false)
    pid
  end

  def run(state, pid, consumer, arg) do
    {:ok, raw} = Circuits.UART.read(pid, 1000)
    String.graphemes(raw)
      |> Enum.reduce(%{state: state}, fn byte, state ->
           consume(byte, state) |> action(consumer, arg)
         end)
    run(state, pid, consumer, arg)
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
  def consumer(s, outlet) do
    case s.type do
      # Midi.note_on ->
      1 ->
        v = outlet.voice_top
        x = s.note
        y = s.note - 12
        z = s.note + 0.1
        IO.puts " ON #{s.channel} #{s.note} #{s.velocity} (#{v})"
        wire("v#{v}n#{x}l5 T v#{v+1}n#{y}l5 T v#{v+2}n#{z}l5 T", outlet)
      # Midi.note_off ->
      2 ->
        v = outlet.voice_top
        IO.puts "OFF #{s.channel} #{s.note} #{s.velocity} (#{v})"
        wire("v#{v}l0 v#{v+1}l0 v#{v+2}l0", outlet)
    end
  end
end

args = System.argv()
port = Enum.at(args, 0)
pid = Midi.open(port)
outlet = Custom.open({0,0,0,0}, 60440)
outlet = Map.merge(outlet, %{voice_top: 40})
Midi.run(0, pid, &Custom.consumer/2, outlet)
