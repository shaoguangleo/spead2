/* Copyright 2015 SKA South Africa
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Semaphore that uses file descriptors, so that it can be plumbed
 * into an event loop.
 */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include "common_features.h"
#include <cerrno>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <atomic>
#include "common_semaphore.h"
#include "common_logging.h"
#if SPEAD2_USE_EVENTFD
# include <sys/eventfd.h>
#endif

namespace spead2
{

[[noreturn]] static void throw_errno()
{
    throw std::system_error(errno, std::system_category());
}

static void log_errno(const char *format)
{
    std::error_code code(errno, std::system_category());
    log_warning(format, code.value(), code.message());
}

semaphore_spin::semaphore_spin(unsigned int initial)
    : value(initial)
{
}

void semaphore_spin::put()
{
    value.fetch_add(1, std::memory_order_release);
}

int semaphore_spin::get()
{
    unsigned int cur = value.load(std::memory_order_acquire);
    while (true)
    {
        if (cur == 0)
            cur = value.load(std::memory_order_acquire);
        else if (value.compare_exchange_weak(
            cur, cur - 1, std::memory_order_acquire))
            break;
    }
    return 0;
}

int semaphore_spin::try_get()
{
    unsigned int cur = value.load(std::memory_order_acquire);
    if (cur > 0
        && value.compare_exchange_strong(
            cur, cur - 1, std::memory_order_acquire))
        return 0;
    else
        return -1;
}

/////////////////////////////////////////////////////////////////////////////

#if SPEAD2_USE_POSIX_SEMAPHORES

semaphore_posix::semaphore_posix(unsigned int initial)
{
    if (sem_init(&sem, 0, initial) == -1)
        throw_errno();
}

semaphore_posix::~semaphore_posix()
{
    if (sem_destroy(&sem) == -1)
    {
        // Destructor, so can't throw
        log_errno("failed to destroy semaphore: %1% (%2%)");
    }
}

void semaphore_posix::put()
{
    if (sem_post(&sem) == -1)
        throw_errno();
}

int semaphore_posix::try_get()
{
    int status = sem_trywait(&sem);
    if (status == -1)
    {
        if (errno == EAGAIN || errno == EINTR)
            return -1;
        else
            throw_errno();
    }
    else
        return 0;
}

int semaphore_posix::get()
{
    int status = sem_wait(&sem);
    if (status == -1)
    {
        if (errno == EINTR)
            return -1;
        else
            throw_errno();
    }
    else
        return 0;
}

#endif // SPEAD2_USE_POSIX_SEMAPHORES

/////////////////////////////////////////////////////////////////////////////

semaphore_pipe::semaphore_pipe(semaphore_pipe &&other)
{
    for (int i = 0; i < 2; i++)
    {
        pipe_fds[i] = other.pipe_fds[i];
        other.pipe_fds[i] = -1;
    }
}

semaphore_pipe &semaphore_pipe::operator=(semaphore_pipe &&other)
{
    for (int i = 0; i < 2; i++)
    {
        if (pipe_fds[i] != -1)
        {
            if (close(pipe_fds[i]) == -1)
                throw_errno();
            pipe_fds[i] = -1;
        }
    }
    for (int i = 0; i < 2; i++)
    {
        pipe_fds[i] = other.pipe_fds[i];
        other.pipe_fds[i] = -1;
    }
    return *this;
}

semaphore_pipe::semaphore_pipe(unsigned int initial)
{
    if (pipe(pipe_fds) == -1)
        throw_errno();
    for (int i = 0; i < 2; i++)
    {
        int flags = fcntl(pipe_fds[i], F_GETFD);
        if (flags == -1)
            throw_errno();
        flags |= FD_CLOEXEC;
        if (fcntl(pipe_fds[i], F_SETFD, flags) == -1)
            throw_errno();
    }
    // Make the read end non-blocking, for try_get
    int flags = fcntl(pipe_fds[0], F_GETFL);
    if (flags == -1)
        throw_errno();
    flags |= O_NONBLOCK;
    if (fcntl(pipe_fds[0], F_SETFL, flags) == -1)
        throw_errno();
    // TODO: this could probably be optimised
    for (unsigned int i = 0; i < initial; i++)
        put();
}

semaphore_pipe::~semaphore_pipe()
{
    for (int i = 0; i < 2; i++)
        if (pipe_fds[i] != -1)
        {
            if (close(pipe_fds[i]) == -1)
            {
                // Can't throw, because this is a destructor
                log_errno("failed to close pipe: %1% (%2%)");
            }
        }
}

void semaphore_pipe::put()
{
    char byte = 0;
    int status;
    do
    {
        status = write(pipe_fds[1], &byte, 1);
        if (status < 0 && errno != EINTR)
        {
            throw_errno();
        }
    } while (status < 0);
}

int semaphore_pipe::get()
{
    char byte = 0;
    while (true)
    {
        struct pollfd pfd = {};
        pfd.fd = pipe_fds[0];
        pfd.events = POLLIN;
        int status = poll(&pfd, 1, -1);
        if (status == -1)
        {
            if (errno == EINTR)
                return -1;
            else
                throw_errno();
        }
        status = read(pipe_fds[0], &byte, 1);
        if (status < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue; // spurious wakeup from poll
            else
                throw_errno();
        }
        else
        {
            assert(status == 1);
            return 0;
        }
    }
}

int semaphore_pipe::try_get()
{
    char byte = 0;
    int status = read(pipe_fds[0], &byte, 1);
    if (status < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        else
            throw_errno();
    }
    else
    {
        assert(status == 1);
        return 0;
    }
}

int semaphore_pipe::get_fd() const
{
    return pipe_fds[0];
}

/////////////////////////////////////////////////////////////////////////////

#if SPEAD2_USE_EVENTFD

semaphore_eventfd::semaphore_eventfd(semaphore_eventfd &&other)
{
    fd = other.fd;
    other.fd = -1;
}

semaphore_eventfd::~semaphore_eventfd()
{
    if (fd != -1 && close(fd) == -1)
        log_errno("failed to close eventfd: %1% (%2%)");
}

semaphore_eventfd &semaphore_eventfd::operator=(semaphore_eventfd &&other)
{
    std::swap(fd, other.fd);
    if (other.fd != -1)
    {
        if (close(other.fd) == -1)
            throw_errno();
        other.fd = -1;
    }
    return *this;
}

semaphore_eventfd::semaphore_eventfd(unsigned int initial)
{
    fd = eventfd(initial, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
    if (fd == -1)
        throw_errno();
}

void semaphore_eventfd::put()
{
    int status;
    do
    {
        status = eventfd_write(fd, 1);
        if (status == -1)
        {
            if (errno != -1)
                throw_errno();
        }
    } while (status == -1);
}

int semaphore_eventfd::try_get()
{
    eventfd_t value;
    int status = eventfd_read(fd, &value);
    if (status == -1)
    {
        if (errno == EAGAIN || errno == EINTR)
            return -1;
        else
            throw_errno();
    }
    assert(status == 0);
    return 0;
}

int semaphore_eventfd::get()
{
    while (true)
    {
        eventfd_t value;
        struct pollfd pfd = {};
        pfd.fd = fd;
        pfd.events = POLLIN;
        int status = poll(&pfd, 1, -1);
        if (status == -1)
        {
            if (errno == EINTR)
                return -1;
            else
                throw_errno();
        }
        status = eventfd_read(fd, &value);
        if (status < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue; // spurious wakeup from poll
            else
                throw_errno();
        }
        else
        {
            assert(status == 0);
            return 0;
        }
    }
}

int semaphore_eventfd::get_fd() const
{
    return fd;
}

#endif // SPEAD2_USE_EVENTFD

} // namespace spead2
