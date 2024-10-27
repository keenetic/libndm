#ifndef __NDM_CORE_HIDDEN_H__
#define __NDM_CORE_HIDDEN_H__

/**
 * During first @c NDM_CORE_INTBLOCK_TIMEOUT_ milliseconds
 * any core I/O operation always is in a non-interruptible state.
 * This allows to send requests or feedbacks while some short period
 * even when a process is in an interrupted state.
 **/
#define NDM_CORE_INTBLOCK_TIMEOUT						1000

#define NDM_CORE_FEEDBACK_SOCKET						"/var/run/ndm.feedback.socket"

/**
 * Send a feedback request to the NDM core and check a core handle result.
 *
 * @param timeout_msec The maximum time allowed to wait, in milliseconds.
 * @param argv An array of environment variables for the feedback to send.
 * It must contain at least one entry with a pointer to the name of
 * the caller, as well as an optional set of additional arguments.
 * The list must be terminated with @c NULL.
 * @param env_argv An array of strings, conventionally of the form key=value,
 * which are passed as feedback environment.
 * The array must be terminated with @c NULL.
 *
 * @returns @c true if request handling is successfull, @c false —
 * otherwise, @a errno stores an error code.
 */

bool ndm_core_send(
		const int fd,
		const struct timespec *intblock,
		const struct timespec *deadline,
		const char *const argv[],
		const char *const env_argv[]);

bool ndm_core_recv(
		const int fd,
		const struct timespec *intblock,
		const struct timespec *deadline);

#endif	/* __NDM_CORE_HIDDEN_H__ */

