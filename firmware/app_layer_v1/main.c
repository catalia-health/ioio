/*
 * Copyright 2011 Ytai Ben-Tsvi. All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ARSHAN POURSOHI OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied.
 */

#include "Compiler.h"
#include "libconn/connection.h"
#include "features.h"
#include "protocol.h"
#include "logging.h"

// define in non-const arrays to ensure data space
static char descManufacturer[] = "IOIO Open Source Project";
static char descModel[] = "IOIO";
static char descDesc[] = "IOIO Standard Application";
static char descVersion[] = FW_IMPL_VER;
static char descUri[] = "https://github.com/ytai/ioio/wiki/ADK";
static char descSerial[] = "N/A";

const char* accessoryDescs[6] = {
  descManufacturer,
  descModel,
  descDesc,
  descVersion,
  descUri,
  descSerial
};

typedef enum {
  STATE_INIT,
  STATE_OPEN_CHANNEL,
  STATE_WAIT_CHANNEL_OPEN,
  STATE_CONNECTED,
  STATE_ERROR
} STATE;

static STATE state = STATE_INIT;
static CHANNEL_HANDLE handle;

void AppCallback(const void* data, UINT32 data_len, int_or_ptr_t arg);

static inline CHANNEL_HANDLE OpenAvailableChannel() {
  int_or_ptr_t arg = { .i = 0 };
  if (ConnectionTypeSupported(CHANNEL_TYPE_ADB)) {
    log_printf("ADB is supported");
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_ADB)) {
      log_printf("ADB can be opened");
      return ConnectionOpenChannelAdb("tcp:4545", &AppCallback, arg);
    }
  } else if (ConnectionTypeSupported(CHANNEL_TYPE_ACC)) {
      log_printf("ACC is supported");
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_ACC)) {
        log_printf("ACC can be opened");
      return ConnectionOpenChannelAccessory(&AppCallback, arg);
    }
  } else if (ConnectionTypeSupported(CHANNEL_TYPE_BT)) {
      log_printf("BT is supported");
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_BT)) {
        log_printf("BT can be opened");
      return ConnectionOpenChannelBtServer(&AppCallback, arg);
    }
  } else if (ConnectionTypeSupported(CHANNEL_TYPE_CDC_DEVICE)) {
      log_printf("CDC is supported");
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_CDC_DEVICE)) {
        log_printf("CDC can be opened");
      return ConnectionOpenChannelCdc(&AppCallback, arg);
    }
  }
  log_printf("can't open channel :(");
  return INVALID_CHANNEL_HANDLE;
}

void AppCallback(const void* data, UINT32 data_len, int_or_ptr_t arg) {
  if (data) {
    if (!AppProtocolHandleIncoming(data, data_len)) {
      // got corrupt input. need to close the connection and soft reset.
      log_printf("Protocol error");
      state = STATE_ERROR;
    }
  } else {
    // connection closed, soft reset and re-establish
    if (state == STATE_CONNECTED) {
      log_printf("Channel closed");
      SoftReset();
    } else {
      log_printf("Channel failed to open");
    }
    state = STATE_OPEN_CHANNEL;
  }
}

int main() {
  int count = 0;
  STATE prev_state = STATE_INIT;
  
  log_init();
  log_printf("***** Hello from app-layer! *******");

  SoftReset();
  ConnectionInit();
  while (1) {
    ConnectionTasks();

    if(state != prev_state) {
        count = 0;
        prev_state = state;
    }

    switch (state) {
      case STATE_INIT:
        log_printf("STATE_INIT");
        count = 0;
        handle = INVALID_CHANNEL_HANDLE;
        state = STATE_OPEN_CHANNEL;
        break;

      case STATE_OPEN_CHANNEL:
        log_printf("STATE_OPEN_CHANNEL");
        if ((handle = OpenAvailableChannel()) != INVALID_CHANNEL_HANDLE) {
          log_printf("Connected, handle = %d", handle);
          state = STATE_WAIT_CHANNEL_OPEN;
        } else {
          count++;
          if (count > 200) {
              state = STATE_INIT;
              ConnectionInit();
              log_printf("Re-initializing from STATE_OPEN_CHANNEL");
          }
        }
        break;

      case STATE_WAIT_CHANNEL_OPEN:
       log_printf("STATE_WAIT_CHANNEL_OPEN");
       log_printf("calling ConnectionCanSend with handle = %d", handle);
       if (ConnectionCanSend(handle)) {
          log_printf("Channel open");
          AppProtocolInit(handle);
          state = STATE_CONNECTED;
        }
        break;

      case STATE_CONNECTED:
        log_printf("STATE_CONNECTED");
        AppProtocolTasks(handle);
        break;

      case STATE_ERROR:
        log_printf("STATE_ERROR");
        ConnectionCloseChannel(handle);
        SoftReset();
        ConnectionInit();
        state = STATE_INIT;
        break;
    }
  }
  return 0;
}
