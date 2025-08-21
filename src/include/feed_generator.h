#pragma once

#include "site_context.h"
#include "list_head.h"
#include "template_engine.h"

void generate_sitemap(SiteContext* s_context, struct list_head* all_posts, TemplateContext* global_context);
void generate_rss_feed(struct list_head* all_posts, TemplateContext* global_context);

