#!/bin/sh
set -e

for test in test_logging_shutdown test_running_thread_pool test_running_stream test_running_chunk_stream_group; do
    echo "Running shutdown test $test"
    python -c "import tests.shutdown; tests.shutdown.$test()"
done
