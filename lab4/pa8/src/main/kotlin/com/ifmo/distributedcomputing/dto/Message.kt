package com.ifmo.distributedcomputing.dto

import com.fasterxml.jackson.annotation.JsonIgnore

data class Message(
  val from: Int,
  val to: Int,
  val type: MessageType,
  var time: Long = -1L
) {
  @JsonIgnore
  fun isBroadcast(): Boolean = (to == -1)
}

enum class MessageType {
  STARTED, DONE, CS_REQUEST, CS_REPLY, CS_RELEASE
}