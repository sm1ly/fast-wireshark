#include "fast.h"
#include "template.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

/* here are our two functions*/
/* parse_xml is the funciton called from packet-fast.c It finds the verious templates and has them parsed. */
void parse_xml(const char *);
/* parse_template is passed a node to a template and tries to read the things inside of it. */
void parse_template(xmlDocPtr, xmlNodePtr, struct template_type*);
void assign_Value(xmlChar *, struct template_field_type *, xmlChar *);

/* Code for parse_xml. Comments below */
void parse_xml(const char* template){
	
	/* 	Our main vairables. the xmlDocPtr doc is a pointer to the document, 
		the xmlNode pointer cur is the current node we are at. We traverse the document like a tree. */
	xmlDocPtr doc;
	xmlNodePtr cur;

	/*****
	read in the document and parse it in one line of code
	*****/
	doc = xmlParseFile(template);
	
	/*****
	Obligatory error checking. note that doc == NULL is an error code
	*****/
	if (doc == NULL ) {
		fprintf(stderr,"Document not parsed successfully. \n");
		return;
	}

	/*****
	Get the where the document starts
	*****/
	cur = xmlDocGetRootElement(doc);
	
	/*****
	More error checking, this in the case of an empty document.
	*****/
	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return;
	}
	/* just some friendly output for testing purposes, this should be removed when code is working right */	
	printf("The root node is %s.\n", (const char *) cur->name);

	/*****
	Begin main parins loop. get the first child of the root, then loop from there. Any "template" nodes imply we have a 		template to parse, so we send it on over to parse_template!
	*****/
	if (cur->name != (const xmlChar *)"template")
		cur = cur->xmlChildrenNode;
	while (cur != NULL) {
/*		printf("Node: %s.\n", cur->name);*/
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"template"))) {
			xmlChar * name;
			xmlChar * id;
			struct template_type * template;
			name = xmlGetProp(cur,(const xmlChar*) "name");
			id = xmlGetProp(cur,(const xmlChar*) "id");
			printf("Name: %s. \nID: %s \n", name, id);
			create_template((const char *)name, atoi((const char*)id), &template);
			parse_template(doc, cur->xmlChildrenNode, template);
			xmlFree(name);
			xmlFree(id);
		}

		cur = cur->next;
	}
	/*****
	Clean up and finish the function
	*****/
	xmlFreeDoc(doc);
	return;


}
/*****
our lovely Parse-template function. This function will eventually build our templates we use, but for now it just tries to traverse them
*****/
void parse_template(xmlDocPtr doc, xmlNodePtr cur, struct template_type* template) {

	/*****
	convience vairable for storing values
	*****/
	xmlChar *name;
	xmlChar *value;
	xmlChar *id;
	xmlChar *presence;
	/*****
	this means we have nothing to do, me thinks
	*****/
/*	if (cur == NULL){
		printf("Well that was a bust, the node is NULL!\n");
	}*/

		/* Time to start on with sorting throguh the nodes. */ 
		while (cur != NULL) {
			/*****
			Find the element of the current node and move into more looping
			*****/		
			struct template_field_type create;			
			guint fop;
			name = xmlGetProp(cur,(const xmlChar*) "name");
			id = xmlGetProp(cur,(const xmlChar*) "id");

			/* set up the field from the current node */
			presence = xmlGetProp(cur,(const xmlChar*) "presence");
			create.name=(char *)cur->name;
			if ((!xmlStrcmp(presence, (const xmlChar *)"optional"))) {	
				create.mandatory=0;
			}
			else if ((!xmlStrcmp(presence, (const xmlChar *)"mandatory"))) {
				create.mandatory=1;
			}
			else {
				create.mandatory=1;
			}
			/* check for fields in subset notes */

			if (cur->xmlChildrenNode != NULL){
				value = xmlGetProp(cur->xmlChildrenNode->next,(const xmlChar*) "value");
				if (value != NULL){
					assign_Value(name, &create, value);
				}
				if ((!xmlStrcmp(cur->xmlChildrenNode->next->name, (const xmlChar *)"default"))) {
					fop=FIELD_OP_DEFAULT;					
				}
				if ((!xmlStrcmp(cur->xmlChildrenNode->next->name, (const xmlChar *)"copy"))) {
					fop=FIELD_OP_COPY;
				}
				if ((!xmlStrcmp(cur->xmlChildrenNode->next->name, (const xmlChar *)"constant"))) {
					fop=FIELD_OP_CONST;
				}
				if ((!xmlStrcmp(cur->xmlChildrenNode->next->name, (const xmlChar *)"increment"))) {
					fop=FIELD_OP_CONST;
				}
				if ((!xmlStrcmp(cur->xmlChildrenNode->next->name, (const xmlChar *)"constant"))) {
					fop=FIELD_OP_CONST;
				}
				if ((!xmlStrcmp(cur->xmlChildrenNode->next->name, (const xmlChar *)"increment"))) {
					fop=FIELD_OP_INCR;
				}
				if ((!xmlStrcmp(cur->xmlChildrenNode->next->name, (const xmlChar *)"delta"))) {
					fop=FIELD_OP_DELTA;
				}
				if ((!xmlStrcmp(cur->xmlChildrenNode->next->name, (const xmlChar *)"tail"))) {
					fop=FIELD_OP_TAIL;
				}
				xmlFree(value);			
/*				printf("SubNodeName: %s \n", cur->xmlChildrenNode->next->name);*/
			}
			/* Determeine Type and create */
			if ((!xmlStrcmp(cur->name, (const xmlChar *)"int32"))) {
				create.type=FIELD_TYPE_INT32|fop;
				create_field(template, &create, NULL);
			}
			if ((!xmlStrcmp(cur->name, (const xmlChar *)"uInt32"))) {
				create.type=FIELD_TYPE_UINT32|fop;
				create_field(template, &create, NULL);
			}
			if ((!xmlStrcmp(cur->name, (const xmlChar *)"int64"))) {
				create.type=FIELD_TYPE_INT64|fop;
				create_field(template, &create, NULL);
			}
			if ((!xmlStrcmp(cur->name, (const xmlChar *)"uInt64"))) {
				create.type=FIELD_TYPE_UINT64|fop;
				create_field(template, &create, NULL);
			}
			if ((!xmlStrcmp(cur->name, (const xmlChar *)"string"))) {
				create.type=FIELD_TYPE_ASCII|fop;
				create_field(template, &create, NULL);
			}
/*			printf("Propname: %s \nnode name: %s \nPropPresence: = %s \nPropid: %s \n", name, cur->name, presence, id);*/

			xmlFree(name);
			xmlFree(presence);
			xmlFree(id);
			cur = cur->next;
		}
	/*****
	all done here, nothing to see, move along now
	*****/
	return;
}

void assign_Value(xmlChar * name, struct template_field_type * create, xmlChar *value){	
	struct template_field_type meow;
	meow = *create;
	if ((!xmlStrcmp(name, (const xmlChar *)"default"))) {
		if ((!xmlStrcmp(name, (const xmlChar *)"int32"))) {
			(*create).cfg_value.i32=(gint32)atoi((const char *)value);
		}
		if ((!xmlStrcmp(name, (const xmlChar *)"uInt32"))) {
			(*create).cfg_value.u32=(gint32)atoi((const char *)value);
		}
		if ((!xmlStrcmp(name, (const xmlChar *)"int64"))) {
			(*create).cfg_value.i64=(gint32)atoi((const char *)value);
		}
		if ((!xmlStrcmp(name, (const xmlChar *)"uInt64"))) {
			(*create).cfg_value.u64=(gint32)atoi((const char *)value);
		}
		if ((!xmlStrcmp(name, (const xmlChar *)"string"))) {
			(*create).cfg_value.str.p=(guint8 *) value;
		}
	}
	if 	((!xmlStrcmp(name, (const xmlChar *)"copy")) 
		|| (!xmlStrcmp(name, (const xmlChar *)"constant"))
		|| (!xmlStrcmp(name, (const xmlChar *)"increment"))
		|| (!xmlStrcmp(name, (const xmlChar *)"constant")) 
		|| (!xmlStrcmp(name, (const xmlChar *)"delta")) 
		|| (!xmlStrcmp(name, (const xmlChar *)"tail"))) {
			if ((!xmlStrcmp(name, (const xmlChar *)"int32"))) {
				(*create).value.i32=(gint32)atoi((const char *)value);
			}
			if ((!xmlStrcmp(name, (const xmlChar *)"uInt32"))) {
				(*create).value.u32=(gint32)atoi((const char *)value);
			}
			if ((!xmlStrcmp(name, (const xmlChar *)"int64"))) {
				(*create).value.i64=(gint32)atoi((const char *)value);
			}
			if ((!xmlStrcmp(name, (const xmlChar *)"uInt64"))) {
				(*create).value.u64=(gint32)atoi((const char *)value);
			}
			if ((!xmlStrcmp(name, (const xmlChar *)"string"))) {
				(*create).value.str.p=(guint8 *) value;
			}					
		}	
}
