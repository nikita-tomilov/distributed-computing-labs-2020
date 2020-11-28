package com.ifmo.distributedcomputing.inbound

import com.fasterxml.jackson.core.JsonFactory
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValues
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
    synchronized(selectionKey) {
      val clientSocket = serverSocket.accept()
      logger.info { "Accepted client connection from ${clientSocket.remoteAddress}" }

      clientSocket.configureBlocking(false);
      clientSocket.register(SelectorSingleton.selector, SelectionKey.OP_READ)

      reactor.registerHandler(ReactorEventType.READ, object : EventHandler {
        override fun handle(selectionKey: SelectionKey) {
          synchronized(selectionKey) {
            val socketChannel = selectionKey.channel() as SocketChannel
            if (socketChannel != clientSocket) return //surprisingly enough, if i get called to read from another socket channel, i fail

            val bb = ByteBuffer.allocate(2048)
            val count = socketChannel.read(bb)
            val bytes = bb.array()

            if ((count == 0) || (bytes[0].toInt() == 0)) return

            var jsonsString = String(bytes).substring(0 until count)
            val openingBracesCount = jsonsString.count { c -> c == '{' }
            val closingBracesCount = jsonsString.count { c -> c == '}' }
            if (openingBracesCount != closingBracesCount) {
              logger.warn { "Mismatch on braces on received string '$jsonsString'" }
              var lastChar = jsonsString.last()
              while (lastChar != '}') {
                val b1 = ByteBuffer.allocate(1)
                val count1 = socketChannel.read(b1)
                if (count1 != 1) continue
                if (b1[0].toInt() == 0) continue
                lastChar = b1.char
                jsonsString += lastChar
              }
            }

            val f = JsonFactory()
            val jsons = mapper.readValues<Message>(f.createParser(jsonsString))
            val messages = jsons.iterator().asSequence().toList()

            messages.sortedBy { it.time }.forEach { msg ->
              onMessageReceived.invoke(msg)
            }
          }
        }
      })
    }
  }

  fun close() {
    serverSocket.close()
  }

  companion object : KLogging()
}