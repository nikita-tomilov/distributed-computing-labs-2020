package com.ifmo.distributedcomputing

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import com.fasterxml.jackson.module.kotlin.registerKotlinModule
import mu.KLogging
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.Socket
import java.util.concurrent.atomic.AtomicBoolean

class IncomingConnection(
  private val clientSocket: Socket,
  private val id: Int
) {

  private val inbound = BufferedReader(InputStreamReader(clientSocket.getInputStream()))
  private val outbound = BufferedWriter(OutputStreamWriter(clientSocket.getOutputStream()))
  private val mapper = ObjectMapper().registerKotlinModule()

  private val isClosed = AtomicBoolean(false)

  private var onReceiveCallback: ((Message) -> Unit) = { }

  private var onCloseCallback: (() -> Unit) = {}

  fun onReceive(callback: ((Message) -> Unit)) {
    this.onReceiveCallback = callback
  }

  fun onClose(callback: (() -> Unit)) {
    this.onCloseCallback = callback
  }

  fun send(m: Message) {
    outbound.write(mapper.writeValueAsString(m) + "\n")
    outbound.flush()
  }

  fun beginListening() {
    logger.info { "Accepted connection from ${clientSocket.remoteSocketAddress}" }
    clientSocket.keepAlive = true
    val t = Thread {
      while (!isClosed.get()) {
        try {
          val string = inbound.readLine()
          if (string != null) {
            onReceiveCallback.invoke(mapper.readValue(string))
          } else {
            close()
          }
        } catch (e: Exception) {
          logger.error(e) { "Error in IncomingConnection" }
        }
      }
    }
    t.name = "child-connection-$id"
    t.isDaemon = true
    t.start()
  }

  private fun close() {
    isClosed.set(true)
    onCloseCallback.invoke()
  }

  companion object : KLogging()
}