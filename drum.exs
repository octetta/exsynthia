defmodule Drum do
  def init do
    {:ok, socket} = :gen_udp.open(0)
    {:ok, socket}
  end

  def wire(msg, socket) do
    :gen_udp.send(socket, {0,0,0,0}, 60440, msg)
    {:ok, socket}
  end

  def beat(tempo) do
    Process.sleep(tempo)
  end

  def kick(socket) do
    wire("v32l10", socket)
  end

  def hh(socket) do
    wire("v34l10", socket)
  end

  def snare(socket) do
    wire("v33l10", socket)
  end

  def drum(tempo, socket) do
    kick(socket)
    beat(tempo)
    hh(socket)
    beat(tempo)
    snare(socket)
    beat(tempo)
    hh(socket)
    beat(tempo)
    drum(tempo, socket)
  end
  
  def loop(tempo) do
    {:ok, socket} = Drum.init
    wire("<p13 <p5 <p11 v33w7p13 v32w7p5 v34w7p11", socket)
    drum(tempo, socket)
  end

  def loop do
    loop(150)
  end
end

# Drum.loop(tempo)
