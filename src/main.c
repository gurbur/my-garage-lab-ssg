#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <time.h>

#include "include/ignore_handler.h"
#include "include/template_engine.h"
#include "include/site_context.h"
#include "include/config_loader.h"
#include "include/build_process.h"
#include "include/file_utils.h"
#include "include/cache_manager.h"
#include "include/hash_table.h"
#include "include/dynamic_buffer.h"
#include "include/feed_generator.h"

#define MAX_PATH_LENGTH 1024

void generate_main_index_page(struct list_head* all_posts, TemplateContext* global_context);
void generate_all_posts_page(struct list_head* all_posts, TemplateContext* global_context);

int main(int argc, char *argv[]) {
	clock_t start_time = clock();

	const char* vault_path = ".";
	if (argc > 1) {
		vault_path = argv[1];
	}

	const char* output_dir = "ssg_output";

	LIST_HEAD(all_posts);

	printf("----- STARTING SSG BUILD -----\n");
	printf("Vault Path: %s\n", vault_path);
	printf("Output Dir: %s\n", output_dir);
	printf("------------------------------\n\n");

	printf("[STEP 1] Loading global context from config.json...\n");
	TemplateContext* global_context = create_template_context();
	load_config("config.json", global_context);

	const char* loaded_title = get_from_context(global_context, "site_title");
	const char* loaded_static_dir = get_from_context(global_context, "build.static_dir");
	const char* loaded_image_dir = get_from_context(global_context, "build.image_dir");
	printf("[DEBUG] Loaded site_title: %s\n", loaded_title ? loaded_title : "Not Found");
	printf("[DEBUG] Loaded build.static_dir: %s\n", loaded_static_dir ? loaded_static_dir : "Not Found");
	printf("[DEBUG] Loaded build.image_dir: %s\n", loaded_image_dir? loaded_image_dir : "Not Found");

	printf("[STEP 3] Preparing for incremental build...\n");
	if (ensure_cache_dir_exists() != 0) {
		fprintf(stderr, "Fatal: Failed to prepare cache directory. Aborting.\n");
		return EXIT_FAILURE;
	}
	HashTable* old_cache = load_cache();
	HashTable* new_cache = ht_create(1024);
	printf("Previous build cache loaded.\n");

	printf("[STEP 4] Scanning vault and creating site context...\n");
	SiteContext* site_context = create_site_context(vault_path);

	printf("[STEP 5] Loading .ssgignore and preparing output directory...\n");
	load_ssgignore(vault_path);

	printf("[STEP 6] Generating sidebar...\n");
	generate_sidebar_html(site_context, global_context);

	printf("[STEP 7] Preparing output directory...\n");
	mkdir(output_dir, 0755);

	build_site(vault_path, site_context, global_context, old_cache, new_cache, &all_posts);

	printf("[STEP 8] Copying static files...\n");
	const char* static_dir = get_from_context(global_context, "build.static_dir");
	if (static_dir && strlen(static_dir) > 0) {
		char dest_static_path[MAX_PATH_LENGTH];
		snprintf(dest_static_path, sizeof(dest_static_path), "%s/%s", output_dir, static_dir);
		copy_static_files(static_dir, dest_static_path);
	} else {
		printf("[INFO] 'build.static_dir' not found in config.json, skipping.\n");
	}

	printf("[STEP 9] Copying image files...\n");
	const char* image_dir = get_from_context(global_context, "build.image_dir");
	if (image_dir && strlen(image_dir) > 0) {
		char dest_image_path[MAX_PATH_LENGTH];
		snprintf(dest_image_path, sizeof(dest_image_path), "%s/%s", output_dir, image_dir);
		copy_static_files(image_dir, dest_image_path);
	} else {
		printf("[INFO] 'build.image_dir' not found in config.json, skipping.\n");
	}

	printf("[STEP 10] Pruning stale files...\n");
	for (size_t i = 0; i < old_cache->size; ++i) {
		HashEntry* entry = old_cache->entries[i];
		while (entry) {
			if (ht_get(new_cache, entry->key) == NULL) {
				char* value_str = (char*)entry->value;
				char* delimiter = strrchr(value_str, ':');
				if (delimiter) {
					char* file_to_delete = delimiter + 1;
					if (remove(file_to_delete) == 0) {
						printf(" - Removed stale file: %s\n", file_to_delete);
					}
				}
			}
			entry = entry->next;
		}
	}

	printf("[STEP 11] Generating main index and all posts page...\n");
	generate_main_index_page(&all_posts, global_context);
	generate_all_posts_page(&all_posts, global_context);

	printf("[STEP 12] Generating sitemap and RSS feed...\n");
	generate_sitemap(site_context, &all_posts, global_context);
	generate_rss_feed(&all_posts, global_context);

	printf("[STEP 13] Cleaning up and saving cache...\n");
	PostSortInfo *post_info, *tmp;
	list_for_each_entry_safe(post_info, tmp, &all_posts, list) {
		list_del(&post_info->list);
		free(post_info->date);
		free(post_info);
	}
	save_cache(new_cache);
	ht_destroy(old_cache, free);
	ht_destroy(new_cache, free);
	free_site_context(site_context);
	free_template_context(global_context);
	free_ignore_patterns();

	clock_t end_time = clock();
	double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;

	printf("---- BUILD FINISHED SUCCESSFULLY! ----\n");
	printf("Total build time: %.3f seconds\n", time_spent);
	printf("--------------------------------------\n");

	return EXIT_SUCCESS;
}

void generate_main_index_page(struct list_head* all_posts, TemplateContext* global_context) {
	int post_count = 0;
	PostSortInfo* p;
	list_for_each_entry(p, all_posts, list) { post_count++; }
	if (post_count == 0) return;

	PostSortInfo* sort_array = malloc(post_count * sizeof(PostSortInfo));
	int i = 0;
	list_for_each_entry(p, all_posts, list) { sort_array[i++] = *p; }
	qsort(sort_array, post_count, sizeof(PostSortInfo), compare_posts);

	DynamicBuffer* recent_posts_buffer = create_dynamic_buffer(1024);
	const char* base_url = get_from_context(global_context, "base_url");
	if (!base_url) base_url = "";

	int recent_count = (post_count > 5) ? 5 : post_count;
	for (i = 0; i < recent_count; i++) {
		TemplateContext* card_context = create_template_context();
		char link_path[MAX_PATH_LENGTH];
		snprintf(link_path, sizeof(link_path), "%s/%s", base_url, sort_array[i].node->slug);

		char* title_from_name = strdup(sort_array[i].node->name);
		char* dot = strrchr(title_from_name, '.');
		if (dot) *dot = '\0';

		add_to_context(card_context, "post_title", title_from_name);
		add_to_context(card_context, "post_link", link_path);

		char* rendered_card = render_template("templates/components/simple_post_item.html", card_context);
		buffer_append_formatted(recent_posts_buffer, "%s", rendered_card);

		free(title_from_name);
		free(rendered_card);
		free_template_context(card_context);
	}

	const char* all_posts_slug = get_from_context(global_context, "all_posts_slug");
	char see_more_link[MAX_PATH_LENGTH];
	snprintf(see_more_link, sizeof(see_more_link), "%s/%s", base_url, all_posts_slug ? all_posts_slug : "posts");

	char* recent_posts_html = destroy_buffer_and_get_content(recent_posts_buffer);
	TemplateContext* page_context = create_template_context();
	copy_context(page_context, global_context);
	add_to_context(page_context, "recent_posts_list", recent_posts_html);
	add_to_context(page_context, "see_more_link", see_more_link);
	add_to_context(page_context, "title", get_from_context(global_context, "site_title"));

	char* content_html = render_template("templates/layout/main_page_layout.html", page_context);
	add_to_context(page_context, "content", content_html);
	char* final_html = render_template("templates/layout/base.html", page_context);

	const char* output_dir = get_from_context(global_context, "build.output_dir");
	char output_path[MAX_PATH_LENGTH];
	snprintf(output_path, sizeof(output_path), "%s/index.html", output_dir ? output_dir : "ssg_output");

	FILE* out_file = fopen(output_path, "w");
	if (out_file) {
		fprintf(out_file, "%s", final_html);
		fclose(out_file);
		printf("  - Generated main index page: %s\n", output_path);
	}

	free(sort_array);
	free(recent_posts_html);
	free(content_html);
	free(final_html);
	free_template_context(page_context);

}

void generate_all_posts_page(struct list_head* all_posts, TemplateContext* global_context) {
	int post_count = 0;
	PostSortInfo* p;
	list_for_each_entry(p, all_posts, list) { post_count++; }
	if (post_count == 0) return;

	PostSortInfo* sort_array = malloc(post_count * sizeof(PostSortInfo));
	int i = 0;
	list_for_each_entry(p, all_posts, list) { sort_array[i++] = *p; }
	qsort(sort_array, post_count, sizeof(PostSortInfo), compare_posts);

	DynamicBuffer* post_list_buffer = create_dynamic_buffer(2048);
	const char* base_url = get_from_context(global_context, "base_url");
	if (!base_url) base_url = "";

	for (i = 0; i < post_count; i++) {
		TemplateContext* card_context = create_template_context();
		char link_path[MAX_PATH_LENGTH];
		snprintf(link_path, sizeof(link_path), "%s/%s", base_url, sort_array[i].node->slug);

		char* title_from_name = strdup(sort_array[i].node->name);
		char* dot = strrchr(title_from_name, '.');
		if (dot) *dot = '\0';

		add_to_context(card_context, "card_item_title", title_from_name);
		add_to_context(card_context, "card_item_link", link_path);
		add_to_context(card_context, "card_item_content", "더보기...");
		char* rendered_card = render_template("templates/components/card.html", card_context);
		buffer_append_formatted(post_list_buffer, "%s", rendered_card);

		free(title_from_name);
		free(rendered_card);
		free_template_context(card_context);
	}

	char* post_list_html = destroy_buffer_and_get_content(post_list_buffer);
	TemplateContext* page_context = create_template_context();
	copy_context(page_context, global_context);
	add_to_context(page_context, "list_title", "모든 글");
	add_to_context(page_context, "title", "모든 글 보기");
	add_to_context(page_context, "post_list_content", post_list_html);

	char breadcrumb_html[MAX_PATH_LENGTH];
	snprintf(breadcrumb_html, sizeof(breadcrumb_html), "<a href=\"%s/\">Home</a> &gt; 모든 글", base_url);
	add_to_context(page_context, "breadcrumb", breadcrumb_html);

	char* content_html = render_template("templates/layout/post_list_layout.html", page_context);
	add_to_context(page_context, "content", content_html);
	char* final_html = render_template("templates/layout/base.html", page_context);

	const char* output_dir = get_from_context(global_context, "build.output_dir");
	const char* posts_page_slug = get_from_context(global_context, "all_posts_slug");
	if (!posts_page_slug) posts_page_slug = "posts";

	char output_path[MAX_PATH_LENGTH];
	snprintf(output_path, sizeof(output_path), "%s/%s.html", output_dir ? output_dir : "ssg_output", posts_page_slug);

	FILE* out_file = fopen(output_path, "w");
	if (out_file) {
		fprintf(out_file, "%s", final_html);
		fclose(out_file);
		printf("  - Generated all posts page: %s\n", output_path);
	}

	free(sort_array);
	free(post_list_html);
	free(content_html);
	free(final_html);
	free_template_context(page_context);
}
