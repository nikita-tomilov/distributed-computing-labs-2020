package com.ifmo.distributedcomputing

import mu.KLogging

object Application : KLogging() {

  @JvmStatic
  fun main(args: Array<String>) {
    if (args.isEmpty()) {
      logger.error { "Please specify N" }
    }
    if (args[0] != "--forked") {
      ParentApplication.parent(args[0].toInt())
    } else {
      ChildrenApplication.child(args[1].toInt(), args[2].toInt(), args[3].toInt())
    }
  }
}
