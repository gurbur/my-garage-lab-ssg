#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/feed_generator.h"
#include "../include/dynamic_buffer.h"
#include "../include/build_process.h"

#define MAX_PATH_LENGTH 1024

static char* get_git_lastmod(const char* file_path) {
	char command[MAX_PATH_LENGTH + 128];
	snprintf(command, sizeof(command), "git log -1 --pretty=format:%%cs -- \"%s\"", file_path);

	FILE* pipe = popen(command, "r");
	if (!pipe) return NULL;

	char buffer[11];
	char* result = NULL;
	if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
		buffer[strcspn(buffer, "\n")] = 0;
		if (strlen(buffer) == 10) result = strdup(buffer);
	}
	pclose(pipe);
	return result;
}

static void get_current_date_str(char* buf, size_t buf_size) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	strftime(buf, buf_size, "%Y-%m-%d", t);
}

static void add_category_urls_to_sitemap_recursively(NavNode* node, DynamicBuffer* db, TemplateContext* global_context, const char* lastmod_date) {
	if (node->is_directory && strlen(node->name) != 0) {
		const char* base_url = get_from_context(global_context, "base_url");
		char category_key[MAX_PATH_LENGTH];
		snprintf(category_key, sizeof(category_key), "category_slugs.%s", node->name);
		const char* category_slug = get_from_context(global_context, category_key);

		if (category_slug) {
			buffer_append_formatted(db, "  <url>\n");
			buffer_append_formatted(db, "    <loc>%s/%s</loc>\n", base_url, category_slug);
			buffer_append_formatted(db, "    <lastmod>%s</lastmod>\n", lastmod_date);
			buffer_append_formatted(db, "  </url>\n");
		}
	}

	NavNode* child;
	list_for_each_entry(child, &node->children, sibling) {
		add_category_urls_to_sitemap_recursively(child, db, global_context, lastmod_date);
	}
}

static void format_date_to_rfc822(const char* date_str, char* buf, size_t buf_size) {
	struct tm tm = {0};

	if (strptime(date_str, "%Y/%m/%d %H:%M", &tm) == NULL) {
		if (strptime(date_str, "%Y-%m-%d", &tm) == NULL) {
			time_t now = time(NULL);
			gmtime_r(&now, &tm);
		}
	}
	tm.tm_isdst = -1;
	strftime(buf, buf_size, "%a, %d %b %Y %H:%M:%S GMT", &tm);
}

void generate_sitemap(SiteContext* s_context, struct list_head* all_posts, TemplateContext* global_context) {
	const char* base_url = get_from_context(global_context, "base_url");
	if (!base_url) return;

	char today_str[11];
	get_current_date_str(today_str, sizeof(today_str));

	DynamicBuffer* db = create_dynamic_buffer(4096);
	buffer_append_formatted(db, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n");

	buffer_append_formatted(db, "  <url>\n    <loc>%s/</loc>\n    <lastmod>%s</lastmod>\n  </url>\n", base_url, today_str);
	const char* all_posts_slug = get_from_context(global_context, "all_posts_slug");
	if (all_posts_slug) {
		buffer_append_formatted(db, "  <url>\n    <loc>%s/%s</loc>\n    <lastmod>%s</lastmod>\n  </url>\n", base_url, all_posts_slug, today_str);
	}
	NavNode* child;
	list_for_each_entry(child, &s_context->root->children, sibling) {
		add_category_urls_to_sitemap_recursively(child, db, global_context, today_str);
	}

	PostSortInfo* p;
	list_for_each_entry(p, all_posts, list) {
		char full_md_path[MAX_PATH_LENGTH];
		snprintf(full_md_path, sizeof(full_md_path), "./%s", p->node->full_path);

		char* git_date = get_git_lastmod(full_md_path);
		const char* final_date = git_date ? git_date : p->date;

		buffer_append_formatted(db, "  <url>\n    <loc>%s/%s</loc>\n    <lastmod>%s</lastmod>\n  </url>\n", base_url, p->node->slug, final_date);

		if (git_date) free(git_date);
	}

	buffer_append_formatted(db, "</urlset>\n");

	const char* output_dir = get_from_context(global_context, "build.output_dir");
	char output_path[MAX_PATH_LENGTH];
	snprintf(output_path, sizeof(output_path), "%s/sitemap.xml", output_dir ? output_dir : "ssg_output");

	char* sitemap_content = destroy_buffer_and_get_content(db);
	FILE* out_file = fopen(output_path, "w");
	if (out_file) {
		fprintf(out_file, "%s", sitemap_content);
		fclose(out_file);
		printf(" - Generated sitemap (using git): %s\n", output_path);
	}
	free(sitemap_content);
}

void generate_rss_feed(struct list_head* all_posts, TemplateContext* global_context) {
	const char* base_url = get_from_context(global_context, "base_url");
	const char* site_title = get_from_context(global_context, "site_title");
	const char* site_description = get_from_context(global_context, "site_description");
	if (!base_url || !site_title) return;

	int post_count = 0;
	PostSortInfo* p;
	list_for_each_entry(p, all_posts, list) { post_count++; }
	if (post_count == 0) return;

	PostSortInfo* sort_array = malloc(post_count * sizeof(PostSortInfo));
	int i = 0;
	list_for_each_entry(p, all_posts, list) { sort_array[i++] = *p; }
	qsort(sort_array, post_count, sizeof(PostSortInfo), compare_posts);

	DynamicBuffer* db = create_dynamic_buffer(4096);
	buffer_append_formatted(db, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<rss version=\"2.0\">\n<channel>\n");
	buffer_append_formatted(db, "  <title>%s</title>\n  <link>%s</link>\n  <description>%s</description>\n", site_title, base_url, site_description ? site_description : "");

	int rss_item_count = (post_count > 20) ? 20 : post_count;
	for (i = 0; i < rss_item_count; i++) {
		char* title_from_name = strdup(sort_array[i].node->name);
		char* dot = strrchr(title_from_name, '.');
		if (dot) *dot = '\0';

		char full_link[MAX_PATH_LENGTH];
		snprintf(full_link, sizeof(full_link), "%s/%s", base_url, sort_array[i].node->slug);

		char pub_date[128];
		format_date_to_rfc822(sort_array[i].date, pub_date, sizeof(pub_date));

		buffer_append_formatted(db, "  <item>\n");
		buffer_append_formatted(db, "    <title>%s</title>\n", title_from_name);
		buffer_append_formatted(db, "    <link>%s</link>\n    <guid isPermaLink=\"true\">%s</guid>\n", full_link, full_link);
		buffer_append_formatted(db, "    <pubDate>%s</pubDate>\n", pub_date);
		buffer_append_formatted(db, "    <description><![CDATA[...]]></description>\n");
		buffer_append_formatted(db, "  </item>\n");

		free(title_from_name);
	}

	buffer_append_formatted(db, "</channel>\n</rss>\n");

	const char* output_dir = get_from_context(global_context, "build.output_dir");
	char output_path[MAX_PATH_LENGTH];
	snprintf(output_path, sizeof(output_path), "%s/rss.xml", output_dir ? output_dir : "ssg_output");

	char* rss_content = destroy_buffer_and_get_content(db);
	FILE* out_file = fopen(output_path, "w");
	if (out_file) {
		fprintf(out_file, "%s", rss_content);
		fclose(out_file);
		printf(" - Generated RSS feed: %s\n", output_path);
	}

	free(rss_content);
	free(sort_array);
}
