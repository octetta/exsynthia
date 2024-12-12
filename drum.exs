defmodule Drum do
  def init do
    {:ok, socket} = :gen_udp.open(0)
    {:ok, socket}
  end

  def wire(msg, socket) do
    :gen_udp.send(socket, {0,0,0,0}, 60440, msg)
    {:ok, socket}
  end

  def beat do
    Process.sleep(150)
  end

  def drum(socket) do
    wire("v32l10", socket)
    Drum.beat
    wire("v34l10", socket)
    Drum.beat
    wire("v33l10", socket)
    Drum.beat
    wire("v34l10", socket)
    Drum.beat
    drum(socket)
  end
  
  def loop do
    {:ok, socket} = Drum.init
    wire("<p13 <p5 <p11 v33w7p13 v32w7p5 v34w7p11", socket)
    drum(socket)
  end
end

Drum.loop
