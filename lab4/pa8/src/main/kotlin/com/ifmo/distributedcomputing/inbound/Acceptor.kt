package com.ifmo.distributedcomputing.inbound

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import com.fasterxml.jackson.module.kotlin.registerKotlinModule
import com.ifmo.distributedcomputing.dto.Message
import com.ifmo.distributedcomputing.ipc.EventHandler
import com.ifmo.distributedcomputing.ipc.Reactor
import com.ifmo.distributedcomputing.ipc.ReactorEventType
import com.ifmo.distributedcomputing.ipc.SelectorSingleton
import mu.KLogging
import java.net.InetSocketAddress
import java.net.StandardSocketOptions
import java.nio.ByteBuffer
import java.nio.channels.SelectionKey
import java.nio.channels.ServerSocketChannel
import java.nio.channels.SocketChannel

class Acceptor(
  private val port: Int,
  private val reactor: Reactor,
  private val onMessageReceived: (Message) -> Unit = {}
) : EventHandler {

  private lateinit var serverSocket: ServerSocketChannel

  private val mapper = ObjectMapper().registerKotlinModule()

  fun setup() {
    serverSocket = ServerSocketChannel.open()
    serverSocket.setOption(StandardSocketOptions.SO_REUSEADDR, true)
    serverSocket.bind(InetSocketAddress(port))
    serverSocket.configureBlocking(false)
    serverSocket.register(SelectorSingleton.selector, SelectionKey.OP_ACCEPT)
    logger.info { "Acceptor opened at $port" }
  }

  override fun handle(selectionKey: SelectionKey) {
    val clientSocket = serverSocket.accept()
    logger.info { "Accepted client connection from ${clientSocket.remoteAddress}" }

    clientSocket.configureBlocking(false);
    clientSocket.register(SelectorSingleton.selector, SelectionKey.OP_READ)

    reactor.registerHandler(ReactorEventType.READ, object : EventHandler {
      override fun handle(selectionKey: SelectionKey) {
        val socketChannel = selectionKey.channel() as SocketChannel
        val bb: ByteBuffer = ByteBuffer.allocate(1024)
        socketChannel.read(bb)
        val bytes = bb.array()
        if (bytes[0].toInt() != 0) {
          val json = String(bytes)
          val msg = mapper.readValue<Message>(json)
          onMessageReceived.invoke(msg)
        }
      }
    })
  }

  fun close() {
    serverSocket.close()
  }

  companion object : KLogging()
}