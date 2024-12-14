#!/bin/env elixir

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

  def bass(0, socket) do
    wire("v0l0v0l0", socket)
  end

  def bass(pos, socket) do
    n = (69-12)+pos
    wire("v0l5n#{n}v1l.1", socket)
  end

  def drum(tempo, pos, socket) do
    IO.puts "bar #{pos}"
    bass(rem(pos,2), socket)
    kick(socket)
    beat(tempo)
    hh(socket)
    beat(tempo)
    snare(socket)
    beat(tempo)
    hh(socket)
    beat(div(tempo,3))
    hh(socket)
    beat(div(tempo,3))
    hh(socket)
    beat(div(tempo,3))
    drum(tempo, pos+1, socket)
  end
  
  def loop(tempo, pos) do
    {:ok, socket} = Drum.init
    wire("<p0 <p1 <p2 <p3 <p4 <p5", socket)
    wire("<p6 <p7 <p8 <p9 <p10 <p11", socket)
    wire("<p12 <p13 <p14 <p15 <p16 <p17", socket)
    wire("<p18 <p19 <p20 <p21 <p22 <p23", socket)
    wire("<p24", socket)
    wire("v1w0f1", socket)
    wire("v0w0f110F1", socket)
    wire("v33w7p13", socket)
    wire("v32w7p5", socket)
    wire("v34w7p11", socket)
    drum(tempo, pos, socket)
  end

  def loop(tempo) do
    loop(tempo,0)
  end

  def loop do
    loop(150,0)
  end
end

# args = System.argv()
# tempo = Enum.at(args, 0)

Drum.loop 200
