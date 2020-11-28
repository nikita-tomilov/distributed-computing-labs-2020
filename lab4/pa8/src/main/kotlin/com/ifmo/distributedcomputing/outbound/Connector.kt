package com.ifmo.distributedcomputing.outbound

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.registerKotlinModule
import com.ifmo.distributedcomputing.dto.Message
import com.ifmo.distributedcomputing.ipc.EventHandler
import com.ifmo.distributedcomputing.ipc.SelectorSingleton
import mu.KLogging
import java.net.InetSocketAddress
import java.net.StandardSocketOptions
import java.nio.ByteBuffer
import java.nio.channels.SelectionKey
import java.nio.channels.SocketChannel
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.atomic.AtomicBoolean

class Connector(
  private val targetHost: String,
  private val targetPort: Int,
  private val targetProcessId: Int,
  private val localPort: Int
) : EventHandler {

  private lateinit var clientSocket: SocketChannel

  private val wasConnected = AtomicBoolean(false)

  private val mapper = ObjectMapper().registerKotlinModule()

  private val writeQueue = ConcurrentLinkedQueue<Message>()

  fun connect() {
    if (wasConnected.get()) return
    try {
      clientSocket = SocketChannel.open()
      clientSocket.setOption(StandardSocketOptions.SO_REUSEADDR, true)
      if (localPort != 0) {
        clientSocket.bind(InetSocketAddress(localPort))
      }
      clientSocket.connect(InetSocketAddress(targetHost, targetPort))
      wasConnected.set(true)
      clientSocket.configureBlocking(false)
      clientSocket.register(SelectorSingleton.selector, SelectionKey.OP_WRITE)
    } catch (e: Exception) {
      //do nothing
      clientSocket.close()
      throw e
    }

    logger.info { "Connected to $targetHost:$targetPort" }
  }

  override fun handle(selectionKey: SelectionKey) {
    synchronized(selectionKey) {
      val channel = selectionKey.channel() as SocketChannel
      val my = clientSocket.socket().channel
      if (channel != my) {
        return
      }
      if (writeQueue.isNotEmpty()) {
        val s = StringBuffer()
        val queueCopy = ArrayList<Message>()
        while (writeQueue.isNotEmpty()) {
          queueCopy.add(writeQueue.poll())
        }
        queueCopy.sortedBy { it.time }.forEach { m ->
          val json = mapper.writeValueAsString(m)
          logger.info { "Sent $m to $targetProcessId" }
          s.append(json)
        }
        val sc = clientSocket.socket().channel
        sc.write(ByteBuffer.wrap(s.toString().toByteArray()))
      }
    }
  }

  fun send(m: Message) {
    if ((m.to == targetProcessId) || (m.isBroadcast())) {
      writeQueue.add(m)
    }
  }

  fun close() {
    clientSocket.close()
  }

  companion object : KLogging()
}