#!/bin/env elixir

Mix.install([
  {:circuits_uart, "~> 1.5"}
])

defmodule Midi do
  @state_none 0
  @state_get_note 10
  @state_get_velocity 20
  @state_get_cc_number 30
  @state_get_cc_value 31
  @state_get_pb_lsb 40
  @state_get_pb_msb 41
  @state_ready 100

  @type_none 0
  @type_note_on 1
  @type_note_off 2
  @type_cc 3
  @type_pb 4
  @type_active 5

  # this message is sent periodically from the keytar
  def consume(<<0xFE>>,state) do
    Map.merge(state, %{state: @state_ready, type: @type_active})
  end
  
  # 0x9x = MIDI on, x = channel, -> get note, followed by velocity
  def consume(<<0x9::4,channel::4>>,state) do
    Map.merge(state, %{state: @state_get_note, type: @type_note_on, channel: channel})
  end
  
  # 0x8x = MIDI off, x = channel, -> get note
  def consume(<<0x8::4,channel::4>>,state) do
    Map.merge(state, %{state: @state_get_note, type: @type_note_off, channel: channel})
  end
  
  # add midi note to map -> get velocity
  def consume(byte,%{state: @state_get_note}=state) do
    <<note>> = byte
    Map.merge(state, %{state: @state_get_velocity, note: note})
  end
  
  # add velocity to map -> ready to be used
  def consume(byte,%{state: @state_get_velocity}=state) do
    <<velocity>> = byte
    Map.merge(state, %{state: @state_ready, velocity: velocity})
  end

  # 0xBx = MIDI control change, -> get cc number, followed by cc value
  def consume(<<0xB::4,channel::4>>,state) do
    Map.merge(state, %{state: @state_get_cc_number, type: @type_cc, channel: channel})
  end
  # add cc number
  def consume(byte, %{state: @state_get_cc_number}=state) do
    <<n>>=byte
    Map.merge(state, %{state: @state_get_cc_value, cc_number: n})
  end
  # add cc value
  def consume(byte, %{state: @state_get_cc_value}=state) do
    <<n>>=byte
    Map.merge(state, %{state: @state_ready, cc_value: n})
  end
  
  # 0xEx = MIDI control change, -> get pitch-bend lsb, followed by msb
  def consume(<<0xE::4,channel::4>>,state) do
    Map.merge(state, %{state: @state_get_pb_lsb, type: @type_pb, channel: channel})
  end
  # add lsb
  def consume(byte, %{state: @state_get_pb_lsb}=state) do
    <<n>>=byte
    Map.merge(state, %{state: @state_get_pb_msb, bend: n})
  end
  # add msb
  def consume(byte, %{state: @state_get_pb_msb}=state) do
    <<n>>=byte
    Map.merge(state, %{state: @state_ready, bend: n*128+state.bend})
  end
  
  def consume(byte,state) do
    # this is a state we don't understand, ignore it and reset
    <<n>> = byte
    IO.puts "i don't know what to do with #{n}"
    IO.inspect state
    %{state: @state_none, type: @type_none}
  end

  def action(%{state: @state_ready}=state, consumer, arg) do
    consumer.(state, arg)
    # Map.merge(state, %{arg: arg})
    state
  end

  def action(state,_,_) do
    state
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
    # IO.inspect s
    case s.type do
      # Midi.note_on ->
      1 ->
        v = outlet.voice_top
        x = s.note
        y = s.note - 12
        z = s.note + 0.1
        IO.puts "  ON #{s.channel} #{s.note} #{s.velocity} (#{v})"
        wire("v#{v}n#{x}l5 T v#{v+1}n#{y}l5 T v#{v+2}n#{z}l5 T", outlet)
      # Midi.note_off ->
      2 ->
        v = outlet.voice_top
        IO.puts " OFF #{s.channel} #{s.note} #{s.velocity} (#{v})"
        wire("v#{v}l0 v#{v+1}l0 v#{v+2}l0", outlet)
      3 ->
        IO.puts "  CC #{s.channel} #{s.cc_number} #{s.cc_value}"
      4 ->
        IO.puts "BEND #{s.channel} #{s.bend}"
      5 ->
        # IO.puts "ACTIVE"
        0
    end
  end
end

args = System.argv()
port = Enum.at(args, 0)
pid = Midi.open(port)
outlet = Custom.open({0,0,0,0}, 60440)
outlet = Map.merge(outlet, %{voice_top: 40})
Midi.run(0, pid, &Custom.consumer/2, outlet)
