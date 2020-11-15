package com.ifmo.distributedcomputing

import mu.KLogging
import java.net.ServerSocket
import java.net.SocketException
import java.net.SocketTimeoutException
import java.util.concurrent.CopyOnWriteArraySet
import java.util.concurrent.CountDownLatch
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger

class Acceptor(
  private val port: Int,
  private val latch: CountDownLatch
) {

  private lateinit var serverSocket: ServerSocket

  private val started = AtomicBoolean(false)

  private val connections = CopyOnWriteArraySet<IncomingConnection>()

  private val id = AtomicInteger(0)

  fun start() {
    if (started.get()) error("already started")
    started.set(true)
    serverSocket = ServerSocket(port)
    serverSocket.soTimeout = 1000
    val serverThread = Thread {
      while (started.get()) {
        try {
          serverAcceptLoop()
        } catch (te: SocketTimeoutException) {
          //do nothing
        } catch (e: Exception) {
          logger.error(e) { "Error in serverThread" }
        }
      }
    }
    serverThread.name = "acceptor"
    serverThread.isDaemon = true
    serverThread.start()
    logger.warn { "Acceptor socket started at port $port" }
  }

  fun stop() {
    started.set(false)
    serverSocket.close()
  }

  private fun serverAcceptLoop() {
    try {
      val clientSocket = serverSocket.accept()
      val connection = IncomingConnection(
          clientSocket,
          id.incrementAndGet())
      connection.onReceive {
        logger.info { "Received: $it" }
        connection.send("ok boomer")
      }
      connection.onClose {
        logger.info { "Connection closed" }
        connections.remove(connection)
        latch.countDown()
      }
      connection.beginListening()
      connections.add(connection)
    } catch (se: SocketException) {
      //do nothing
    }
  }

  companion object : KLogging()
}