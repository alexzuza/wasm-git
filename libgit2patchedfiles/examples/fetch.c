#include "common.h"

static int merge(git_repository* repo, char str[]);

static int progress_cb(const char *str, int len, void *data)
{
	(void)data;
	printf("remote: %.*s", len, str);
	fflush(stdout); /* We don't have the \n to force the flush */
	return 0;
}

/**
 * This function gets called for each remote-tracking branch that gets
 * updated. The message we output depends on whether it's a new one or
 * an update.
 */
static int update_cb(const char *refname, const git_oid *a, const git_oid *b, void *data)
{
	char a_str[GIT_OID_HEXSZ+1], b_str[GIT_OID_HEXSZ+1];
	(void)data;

	git_oid_fmt(b_str, b);
	b_str[GIT_OID_HEXSZ] = '\0';

	if (git_oid_is_zero(a)) {
		printf("[new]     %.20s %s\n", b_str, refname);
	} else {
		git_oid_fmt(a_str, a);
		a_str[GIT_OID_HEXSZ] = '\0';
		printf("[updated] %.10s..%.10s %s\n", a_str, b_str, refname);
	}

	return 0;
}

/**
 * This gets called during the download and indexing. Here we show
 * processed and total objects in the pack and the amount of received
 * data. Most frontends will probably want to show a percentage and
 * the download rate.
 */
static int transfer_progress_cb(const git_indexer_progress *stats, void *payload)
{
	(void)payload;

	if (stats->received_objects == stats->total_objects) {
		printf("Resolving deltas %u/%u\r",
		       stats->indexed_deltas, stats->total_deltas);
	} else if (stats->total_objects > 0) {
		printf("Received %u/%u objects (%u) in %" PRIuZ " bytes\r",
		       stats->received_objects, stats->total_objects,
		       stats->indexed_objects, stats->received_bytes);
	}
	return 0;
}

/** Entry point for this command */
int lg2_fetch(git_repository *repo, int argc, char **argv)
{
	git_remote *remote = NULL;
	const git_indexer_progress *stats;
	git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
	struct args_info args = ARGS_INFO_INIT;
	char* input = NULL;

	if (argc < 2) {
		fprintf(stderr, "usage: %s fetch <repo>\n", argv[-1]);
		return EXIT_FAILURE;
	}

	if (!strcmp(".", argv[1]))
	{
		// https://stackoverflow.com/questions/38915026/how-is-fast-forward-implemented-in-git-so-that-two-branches-preserve
		// git fetch . <sourceBranch>:<destinationBranch>
		git_remote_free(remote);

		for (args.pos = 2; args.pos < argc; ++args.pos)
		{
			input = (char*)malloc(strlen(argv[args.pos]));
			if (input)
			{
				strcpy(input, argv[args.pos]);

				if (merge(repo, input) != 0)
					return -1;
			}
		}

		return 0;
	}

	/* Figure out whether it's a named remote or a URL */
	printf("Fetching %s for repo %p\n", argv[1], repo);
	if (git_remote_lookup(&remote, repo, argv[1]) < 0)
		if (git_remote_create_anonymous(&remote, repo, argv[1]) < 0)
			goto on_error;

	/* Set up the callbacks (only update_tips for now) */
	fetch_opts.callbacks.update_tips = &update_cb;
	fetch_opts.callbacks.sideband_progress = &progress_cb;
	fetch_opts.callbacks.transfer_progress = transfer_progress_cb;
	fetch_opts.callbacks.credentials = cred_acquire_cb;

	/**
	 * Perform the fetch with the configured refspecs from the
	 * config. Update the reflog for the updated references with
	 * "fetch".
	 */
	if (git_remote_fetch(remote, NULL, &fetch_opts, "fetch") < 0)
		goto on_error;

	/**
	 * If there are local objects (we got a thin pack), then tell
	 * the user how many objects we saved from having to cross the
	 * network.
	 */
	stats = git_remote_stats(remote);
	if (stats->local_objects > 0) {
		printf("\rReceived %u/%u objects in %" PRIuZ " bytes (used %u local objects)\n",
			stats->indexed_objects, stats->total_objects, stats->received_bytes, stats->local_objects);
	} else{
		printf("\rReceived %u/%u objects in %" PRIuZ "bytes\n",
			stats->indexed_objects, stats->total_objects, stats->received_bytes);
	}

	git_remote_free(remote);

	return 0;

 on_error:
	git_remote_free(remote);
	return -1;
}

static int merge(git_repository* repo, char str[])
{
	git_merge_analysis_t analysis_out;
	git_merge_preference_t preference_out;
	git_reference* head_ref = NULL;
	git_reference* new_target_ref = NULL;
	char* from = NULL;
	char* to = NULL;
	git_annotated_commit** annotated = calloc(1, sizeof(git_annotated_commit*));
	int error = 0;

	from = strtok(str, ":");
	error = git_reference_lookup(&head_ref, repo, from);
	if (error != 0)
		goto on_error;

	to = strtok(0, ":");
	error = resolve_refish(&annotated[0], repo, to);
	if (error != 0 || !annotated)
		goto on_error;

	error = git_merge_analysis_for_ref(&analysis_out, &preference_out, repo, head_ref, annotated, 1);
	if (error != 0)
		goto on_error;

	if (analysis_out & GIT_MERGE_ANALYSIS_FASTFORWARD)
	{
		// fast forward
		const git_oid* target_oid;
		/* Since this is a fast-forward, there can be only one merge head */
		target_oid = git_annotated_commit_id(annotated[0]);
		/* Move the target reference to the target OID */
		error = git_reference_set_target(&new_target_ref, head_ref, target_oid, NULL);
		if (error != 0)
		{
			printf("failed to move reference\n");
			goto on_error;
		}
		printf("successfully fast-forward %s to %s\n", from, to);
	}

	git_reference_free(head_ref);
	git_reference_free(new_target_ref);
	free(annotated);

	return 0;

on_error:
	printf("fast-forward merge failed: %s\n", git_error_last()->message);
	git_reference_free(head_ref);
	git_reference_free(new_target_ref);
	free(annotated);

	return -1;
}
