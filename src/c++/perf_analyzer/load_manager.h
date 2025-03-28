// Copyright 2020-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <random>
#include <thread>

#include "client_backend/client_backend.h"
#include "data_loader.h"
#include "infer_data_manager.h"
#include "load_worker.h"
#include "perf_utils.h"
#include "sequence_manager.h"

namespace triton { namespace perfanalyzer {

class LoadManager {
 public:
  virtual ~LoadManager() = default;

  /// Initialize the Manager class to set up shared memory and inputs
  /// \param string_length The length of the random strings to be generated
  /// for string inputs.
  /// \param string_data The string to be used as string inputs for model.
  /// \param zero_input Whether to use zero for model inputs.
  /// \param user_data The vector containing path/paths to user-provided data
  /// that can be a directory or path to a json data file.
  /// \param start_sequence_id The starting sequence ID to be used for iterating
  /// through valid sequence IDs.
  /// \param sequence_id_range The maximum sequence ID to be used for iterating
  /// through valid sequence IDs.
  /// \param sequence_length The base length of new sequences.
  void InitManager(
      const size_t string_length, const std::string& string_data,
      const bool zero_input, std::vector<std::string>& user_data,
      const uint64_t start_sequence_id, const uint64_t sequence_id_range,
      const size_t sequence_length);

  /// Check if the load manager is working as expected.
  /// \return cb::Error object indicating success or failure.
  cb::Error CheckHealth();

  /// Swap the content of the timestamp vector recorded by the load
  /// manager with a new timestamp vector
  /// \param new_timestamps The timestamp vector to be swapped.
  /// \return cb::Error object indicating success or failure.
  cb::Error SwapTimestamps(TimestampVector& new_timestamps);

  /// Get the sum of all contexts' stat
  /// \param contexts_stat Returned the accumulated stat from all contexts
  /// in load manager
  cb::Error GetAccumulatedClientStat(cb::InferStat* contexts_stat);

  /// \return the batch size used for the inference requests
  size_t BatchSize() const { return batch_size_; }

  /// Resets all worker thread states to beginning of schedule.
  /// \return cb::Error object indicating success or failure.
  virtual cb::Error ResetWorkers()
  {
    return cb::Error(
        "resetting worker threads not supported for this load manager.",
        pa::GENERIC_ERROR);
  }

  /// Count the number of requests collected until now.
  uint64_t CountCollectedRequests();

 protected:
  LoadManager(
      const bool async, const bool streaming, const int32_t batch_size,
      const size_t max_threads, const SharedMemoryType shared_memory_type,
      const size_t output_shm_size, const std::shared_ptr<ModelParser>& parser,
      const std::shared_ptr<cb::ClientBackendFactory>& factory);

  /// Complete any subclass-specific manager initialization tasks.
  virtual void InitManagerFinalize() {}

  /// Helper funtion to retrieve the input data for the inferences
  /// \param string_length The length of the random strings to be generated
  /// for string inputs.
  /// \param string_data The string to be used as string inputs for model.
  /// \param zero_input Whether to use zero for model inputs.
  /// \param user_data The vector containing path/paths to user-provided data
  /// that can be a directory or path to a json data file.
  /// \return cb::Error object indicating success or failure.
  cb::Error InitManagerInputs(
      const size_t string_length, const std::string& string_data,
      const bool zero_input, std::vector<std::string>& user_data);

  /// Stops all the worker threads generating the request load.
  void StopWorkerThreads();

 protected:
  bool async_;
  bool streaming_;
  size_t batch_size_;
  size_t max_threads_;
  bool on_sequence_model_;

  std::shared_ptr<ModelParser> parser_;
  std::shared_ptr<cb::ClientBackendFactory> factory_;

  bool using_json_data_;

  std::shared_ptr<DataLoader> data_loader_;
  std::unique_ptr<cb::ClientBackend> backend_;
  std::shared_ptr<InferDataManager> infer_data_manager_;

  // Track the workers so they all go out of scope at the
  // same time
  std::vector<std::shared_ptr<IWorker>> workers_;

  // Worker threads that loads the server with inferences
  std::vector<std::thread> threads_;
  // Contains the statistics on the current working threads
  std::vector<std::shared_ptr<ThreadStat>> threads_stat_;

  // Use condition variable to pause/continue worker threads
  std::condition_variable wake_signal_;
  std::mutex wake_mutex_;

  std::shared_ptr<SequenceManager> sequence_manager_{nullptr};

#ifndef DOCTEST_CONFIG_DISABLE
 protected:
  LoadManager() = default;
#endif
};

}}  // namespace triton::perfanalyzer
