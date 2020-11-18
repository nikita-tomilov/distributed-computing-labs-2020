package com.ifmo.distributedcomputing.ipc

import java.nio.channels.Selector

object SelectorSingleton {
  val selector: Selector = Selector.open()
}