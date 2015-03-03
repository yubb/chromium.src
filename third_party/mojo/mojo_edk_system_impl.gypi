# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# The dictionary here is defined for use by the "mojo_system_impl" and
# "mojo_system_impl_win64" targets in third_party/mojo/mojo_edk.gyp. It's defined in this
# .gypi file so the sections aren't duplicated.
{
  'defines': [
    'MOJO_SYSTEM_IMPL_IMPLEMENTATION',
    'MOJO_SYSTEM_IMPLEMENTATION',
    'MOJO_USE_SYSTEM_IMPL',
  ],
  'sources': [
    'src/mojo/edk/embedder/configuration.h',
    'src/mojo/edk/embedder/channel_info_forward.h',
    'src/mojo/edk/embedder/embedder.cc',
    'src/mojo/edk/embedder/embedder.h',
    'src/mojo/edk/embedder/embedder_internal.h',
    'src/mojo/edk/embedder/entrypoints.cc',
    'src/mojo/edk/embedder/platform_channel_pair.cc',
    'src/mojo/edk/embedder/platform_channel_pair.h',
    'src/mojo/edk/embedder/platform_channel_pair_posix.cc',
    'src/mojo/edk/embedder/platform_channel_pair_win.cc',
    'src/mojo/edk/embedder/platform_channel_utils_posix.cc',
    'src/mojo/edk/embedder/platform_channel_utils_posix.h',
    'src/mojo/edk/embedder/platform_handle.cc',
    'src/mojo/edk/embedder/platform_handle.h',
    'src/mojo/edk/embedder/platform_handle_utils.h',
    'src/mojo/edk/embedder/platform_handle_utils_posix.cc',
    'src/mojo/edk/embedder/platform_handle_utils_win.cc',
    'src/mojo/edk/embedder/platform_handle_vector.h',
    'src/mojo/edk/embedder/platform_shared_buffer.h',
    'src/mojo/edk/embedder/platform_support.h',
    'src/mojo/edk/embedder/scoped_platform_handle.h',
    'src/mojo/edk/embedder/simple_platform_shared_buffer.cc',
    'src/mojo/edk/embedder/simple_platform_shared_buffer.h',
    'src/mojo/edk/embedder/simple_platform_shared_buffer_android.cc',
    'src/mojo/edk/embedder/simple_platform_shared_buffer_posix.cc',
    'src/mojo/edk/embedder/simple_platform_shared_buffer_win.cc',
    'src/mojo/edk/embedder/simple_platform_support.cc',
    'src/mojo/edk/embedder/simple_platform_support.h',
    'src/mojo/edk/system/awakable.h',
    'src/mojo/edk/system/awakable_list.cc',
    'src/mojo/edk/system/awakable_list.h',
    'src/mojo/edk/system/async_waiter.cc',
    'src/mojo/edk/system/async_waiter.h',
    'src/mojo/edk/system/channel.cc',
    'src/mojo/edk/system/channel.h',
    'src/mojo/edk/system/channel_endpoint.cc',
    'src/mojo/edk/system/channel_endpoint.h',
    'src/mojo/edk/system/channel_endpoint_client.h',
    'src/mojo/edk/system/channel_endpoint_id.cc',
    'src/mojo/edk/system/channel_endpoint_id.h',
    'src/mojo/edk/system/channel_info.cc',
    'src/mojo/edk/system/channel_info.h',
    'src/mojo/edk/system/channel_manager.cc',
    'src/mojo/edk/system/channel_manager.h',
    'src/mojo/edk/system/configuration.cc',
    'src/mojo/edk/system/configuration.h',
    'src/mojo/edk/system/core.cc',
    'src/mojo/edk/system/core.h',
    'src/mojo/edk/system/data_pipe.cc',
    'src/mojo/edk/system/data_pipe.h',
    'src/mojo/edk/system/data_pipe_impl.h',
    'src/mojo/edk/system/data_pipe_consumer_dispatcher.cc',
    'src/mojo/edk/system/data_pipe_consumer_dispatcher.h',
    'src/mojo/edk/system/data_pipe_producer_dispatcher.cc',
    'src/mojo/edk/system/data_pipe_producer_dispatcher.h',
    'src/mojo/edk/system/dispatcher.cc',
    'src/mojo/edk/system/dispatcher.h',
    'src/mojo/edk/system/endpoint_relayer.cc',
    'src/mojo/edk/system/endpoint_relayer.h',
    'src/mojo/edk/system/handle_signals_state.h',
    'src/mojo/edk/system/handle_table.cc',
    'src/mojo/edk/system/handle_table.h',
    'src/mojo/edk/system/incoming_endpoint.cc',
    'src/mojo/edk/system/incoming_endpoint.h',
    'src/mojo/edk/system/local_data_pipe_impl.cc',
    'src/mojo/edk/system/local_data_pipe_impl.h',
    'src/mojo/edk/system/local_message_pipe_endpoint.cc',
    'src/mojo/edk/system/local_message_pipe_endpoint.h',
    'src/mojo/edk/system/mapping_table.cc',
    'src/mojo/edk/system/mapping_table.h',
    'src/mojo/edk/system/master_connection_manager.cc',
    'src/mojo/edk/system/master_connection_manager.h',
    'src/mojo/edk/system/memory.cc',
    'src/mojo/edk/system/memory.h',
    'src/mojo/edk/system/message_in_transit.cc',
    'src/mojo/edk/system/message_in_transit.h',
    'src/mojo/edk/system/message_in_transit_queue.cc',
    'src/mojo/edk/system/message_in_transit_queue.h',
    'src/mojo/edk/system/message_pipe.cc',
    'src/mojo/edk/system/message_pipe.h',
    'src/mojo/edk/system/message_pipe_dispatcher.cc',
    'src/mojo/edk/system/message_pipe_dispatcher.h',
    'src/mojo/edk/system/message_pipe_endpoint.cc',
    'src/mojo/edk/system/message_pipe_endpoint.h',
    'src/mojo/edk/system/options_validation.h',
    'src/mojo/edk/system/platform_handle_dispatcher.cc',
    'src/mojo/edk/system/platform_handle_dispatcher.h',
    'src/mojo/edk/system/proxy_message_pipe_endpoint.cc',
    'src/mojo/edk/system/proxy_message_pipe_endpoint.h',
    'src/mojo/edk/system/raw_channel.cc',
    'src/mojo/edk/system/raw_channel.h',
    'src/mojo/edk/system/raw_channel_posix.cc',
    'src/mojo/edk/system/raw_channel_win.cc',
    'src/mojo/edk/system/shared_buffer_dispatcher.cc',
    'src/mojo/edk/system/shared_buffer_dispatcher.h',
    'src/mojo/edk/system/simple_dispatcher.cc',
    'src/mojo/edk/system/simple_dispatcher.h',
    'src/mojo/edk/system/slave_connection_manager.cc',
    'src/mojo/edk/system/slave_connection_manager.h',
    'src/mojo/edk/system/transport_data.cc',
    'src/mojo/edk/system/transport_data.h',
    'src/mojo/edk/system/unique_identifier.cc',
    'src/mojo/edk/system/unique_identifier.h',
    'src/mojo/edk/system/waiter.cc',
    'src/mojo/edk/system/waiter.h',
    # Test-only code:
    # TODO(vtl): It's a little unfortunate that these end up in the same
    # component as non-test-only code. In the static build, this code
    # should hopefully be dead-stripped.
    'src/mojo/edk/embedder/test_embedder.cc',
    'src/mojo/edk/embedder/test_embedder.h',
  ],
  'all_dependent_settings': {
    # Ensures that dependent projects import the core functions on Windows.
    'defines': ['MOJO_USE_SYSTEM_IMPL'],
  },
  'conditions': [
    ['OS=="android"', {
      'dependencies': [
        '<(DEPTH)/third_party/ashmem/ashmem.gyp:ashmem',
      ],
    }],
  ],
}
