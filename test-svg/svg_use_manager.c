// svg_use_manager.c
#include "svg_use_manager.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <string.h>
#include <stdlib.h>

struct SvgUseManager {
    xmlDocPtr doc;
    xmlNodePtr root;
    struct LookupEntry {
        char *url;
        xmlNodePtr g_node;
    } *lookup;
    int lookup_count;
};

static void process_use_nodes(SvgUseManager *mgr) {
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(mgr->doc);
    xmlXPathRegisterNs(xpathCtx, (const xmlChar *)"svg", (const xmlChar *)"http://www.w3.org/2000/svg");
    xmlXPathRegisterNs(xpathCtx, (const xmlChar *)"xlink", (const xmlChar *)"http://www.w3.org/1999/xlink");
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar*)"//svg:use", xpathCtx);

    if(xpathObj && xpathObj->nodesetval) {
        int count = xpathObj->nodesetval->nodeNr;
        mgr->lookup = calloc(count, sizeof(*mgr->lookup));
        mgr->lookup_count = 0;

        for(int i = 0; i < count; i++) {
            xmlNodePtr use_node = xpathObj->nodesetval->nodeTab[i];
            xmlChar *href = xmlGetProp(use_node, (const xmlChar*)"xlink:href");
            if(!href) href = xmlGetProp(use_node, (const xmlChar*)"href");
            if(!href) continue;

            // Create a <g> node to replace <use>
            xmlNodePtr g_node = xmlNewNode(NULL, (const xmlChar*)"g");

            // Copy attributes (except href)
            for(xmlAttrPtr attr = use_node->properties; attr; attr = attr->next) {
                if(strcmp((const char*)attr->name, "xlink:href") != 0 && strcmp((const char*)attr->name, "href") != 0)
                    xmlSetProp(g_node, attr->name, attr->children->content);
            }

            xmlReplaceNode(use_node, g_node);
            xmlFreeNode(use_node);

            mgr->lookup[mgr->lookup_count].url = strdup((const char*)href);
            mgr->lookup[mgr->lookup_count].g_node = g_node;
            mgr->lookup_count++;

            xmlFree(href);
        }
    }
    if(xpathObj) xmlXPathFreeObject(xpathObj);
    if(xpathCtx) xmlXPathFreeContext(xpathCtx);
}

SvgUseManager* svg_use_manager_create(const char *svg_content) {
    xmlDocPtr doc = xmlParseMemory(svg_content, strlen(svg_content));
    if(!doc) return NULL;

    SvgUseManager *mgr = calloc(1, sizeof(SvgUseManager));
    mgr->doc = doc;
    mgr->root = xmlDocGetRootElement(doc);

    process_use_nodes(mgr);
    return mgr;
}

void svg_use_manager_free(SvgUseManager *mgr) {
    if(!mgr) return;
    for(int i = 0; i < mgr->lookup_count; i++) {
        free(mgr->lookup[i].url);
    }
    free(mgr->lookup);
    if(mgr->doc) xmlFreeDoc(mgr->doc);
    free(mgr);
}

char** svg_use_manager_list_urls(SvgUseManager *mgr) {
    char **urls = calloc(mgr->lookup_count + 1, sizeof(char*));
    for(int i = 0; i < mgr->lookup_count; i++)
        urls[i] = strdup(mgr->lookup[i].url);
    urls[mgr->lookup_count] = NULL;
    return urls;
}


int svg_use_manager_replace(SvgUseManager *mgr, const char *url, const char *new_svg) {
    for (int i = 0; i < mgr->lookup_count; i++) {
        if (strcmp(mgr->lookup[i].url, url) == 0) {
            // Wrap the fragment in a temporary root
            char *wrapped_svg = malloc(strlen(new_svg) + 20);
            if (!wrapped_svg) return -1;
            sprintf(wrapped_svg, "<root>%s</root>", new_svg);

            xmlDocPtr new_doc = xmlParseMemory(wrapped_svg, strlen(wrapped_svg));
            free(wrapped_svg);

            if (!new_doc) return -1;
            xmlNodePtr new_root = xmlDocGetRootElement(new_doc); // this is <root>

            // Remove existing children of g_node
            xmlNodePtr child, next;
            for (child = mgr->lookup[i].g_node->children; child; child = next) {
                next = child->next;
                xmlUnlinkNode(child);
                xmlFreeNode(child);
            }

            // Import each child of <root> into the g_node
            for (child = new_root->children; child; child = child->next) {
                xmlNodePtr imported = xmlDocCopyNode(child, mgr->doc, 1);
                xmlAddChild(mgr->lookup[i].g_node, imported);
            }

            xmlFreeDoc(new_doc);
            return 0;
        }
    }
    return -1; // URL not found
}

char* svg_use_manager_render(SvgUseManager *mgr) {
    xmlChar *xmlbuff;
    int buffersize;
    xmlDocDumpFormatMemoryEnc(mgr->doc, &xmlbuff, &buffersize, "UTF-8", 1);
    char *result = strndup((const char*)xmlbuff, buffersize);
    xmlFree(xmlbuff);
    return result;
}
