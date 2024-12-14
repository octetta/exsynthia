Mix.install([
  {:circuits_uart, "~> 1.5"}
])

# u = Circuits.UART.enumerate
# IO.inspect u

{:ok, pid} = Circuits.UART.start_link
IO.inspect pid

args = System.argv()
IO.inspect args

port = Enum.at(args, 0)

Circuits.UART.open(pid, port, speed: 31250, active: false)

# {:ok, s} = Circuits.UART.read(pid, 1000)
# IO.inspect s


defmodule Midi do
  @_none 0
  @_noteon 1
  @_noteoff 2
  @_velocity 3
  @_ready 100

  @active  254
  @noteon  144
  @noteoff 128

  def decide(@active,_,_) do
    %{state: @_none, mode: @_none}
  end
  
  def decide(@noteon,_,_) do
    %{state: @_noteon, mode: @_none}
  end
  
  def decide(@noteoff,_,_) do
    %{state: @_noteoff, mode: @_none}
  end
  
  def decide(val,@_noteon,_) do
    %{state: @_velocity, mode: @_noteon, note: val}
  end
  
  def decide(val,@_noteoff,_) do
    %{state: @_velocity, mode: @_noteoff, note: val}
  end
  
  def decide(val,@_velocity,prior) do
    Map.merge(prior, %{state: @_ready, velocity: val})
  end
  
  def decide(_,_,_) do
    # this is a state we don't understand, ignore it and reset
    %{state: @_none, mode: @_none}
  end

  def action(@_ready, s) do
    IO.inspect s
  end

  def action(_,_) do
  end

  def loop(val, current) do
    s = decide(val, current.state, current)
    action(s.state, s)
    s
  end

  def read(state, pid) do
    {:ok, raw} = Circuits.UART.read(pid, 1000)
    l = :binary.bin_to_list(raw)
    Enum.reduce(l, %{state: state}, fn val, state -> loop(val, state) end)
    read(state, pid)
  end
end

IO.puts "Midi.read(0, pid)"
Midi.read(0, pid)
