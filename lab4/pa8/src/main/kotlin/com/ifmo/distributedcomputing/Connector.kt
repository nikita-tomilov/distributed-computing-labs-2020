package com.ifmo.distributedcomputing

import mu.KLogging
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.lang.Exception
import java.net.BindException
import java.net.ConnectException
import java.net.InetSocketAddress
import java.net.Socket
import java.util.concurrent.atomic.AtomicBoolean

class Connector(
  private val targetHost: String,
  private val targetPort: Int,
  private val localPort: Int
) {

  private lateinit var clientSocket: Socket

  private lateinit var fromServer: BufferedReader

  private lateinit var toServer: BufferedWriter

  private val wasConnected = AtomicBoolean(false)

  fun connect() {
    while (!wasConnected.get()) {
      try {
        clientSocket = Socket()
        if (localPort != 0) {
          clientSocket.bind(InetSocketAddress(localPort))
        }
        clientSocket.connect(InetSocketAddress(targetHost, targetPort))
        wasConnected.set(true)
        fromServer = BufferedReader(InputStreamReader(clientSocket.getInputStream()))
        toServer = BufferedWriter(OutputStreamWriter(clientSocket.getOutputStream()))
      } catch (e: ConnectException) {
        //do nothing
        clientSocket.close()
        logger.warn { "ConnectException" }
        Thread.sleep(1000)
      } catch (e: BindException) {
        //do nothing as well
        clientSocket.close()
        logger.warn { "BindException" }
        Thread.sleep(1000)
      } catch (e: Exception) {
        logger.error(e) { "Error when connecting to $targetHost:$targetPort from local port $localPort" }
        clientSocket.close()
        Thread.sleep(1000)
      }
    }
    logger.warn { "Connected to $targetHost:$targetPort" }
  }

  fun close() {
    clientSocket.shutdownOutput()
    clientSocket.shutdownInput()
    clientSocket.close()
  }

  fun send(s: String) {
    toServer.write(s + "\n")
    toServer.flush()
  }

  fun receiveBlocking(): String = fromServer.readLine()

  companion object : KLogging()
}