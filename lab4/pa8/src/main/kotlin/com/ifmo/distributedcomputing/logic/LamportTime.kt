package com.ifmo.distributedcomputing.logic

class LamportTime {

  private var time = 0L

  @Synchronized
  fun get() = time

  @Synchronized
  fun update(another: Long): Long {
    val current = time
    if (current > another) {
      time = another
    }
    return time
  }

  @Synchronized
  fun updateAndIncrement(another: Long): Long {
    val current = time
    if (current > another) {
      time = another
    }
    time++
    return time
  }
}