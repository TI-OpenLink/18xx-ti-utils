/*
 * Copyright (C) 2012 Texas Instruments Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <regex.h>
#include <sys/stat.h>
#include <getopt.h>

struct element {
	char *name;
	int type;
	int array_size;
	int *value;
	size_t position;
};

struct structure {
	char *name;
	int n_elements;
	struct element *elements;
	size_t size;
};

struct type {
	char *name;
	int size;
	char *format;
};

struct type types[] = {
	{ "u32", 4, "%u" },
	{ "u16", 2, "%u" },
	{ "u8",  1, "%u" },
	{ "s32", 4, "%d" },
	{ "s16", 2, "%d" },
	{ "s8",  1, "%d" },
	{ "__le32", 4, "%d" },
	{ "__le16", 2, "%d" },
};

#define DEFAULT_INPUT_FILENAME	"wl18xx-conf-default.bin"
#define DEFAULT_OUTPUT_FILENAME	"wl18xx-conf.bin"
#define DEFAULT_BIN_FILENAME	"struct.bin"
#define DEFAULT_ROOT_STRUCT	"wlcore_conf_file"
#define DEFAULT_MAGIC_SYMBOL	"WL18XX_CONF_MAGIC"
#define DEFAULT_VERSION_SYMBOL	"WL18XX_CONF_VERSION"
#define DEFAULT_MAGIC_ELEMENT	"header.magic"
#define DEFAULT_VERSION_ELEMENT	"header.version"
#define DEFAULT_CHKSUM_ELEMENT	"header.checksum"
#define MAX_INDENT		8
#define INDENT_CHAR		"\t"

#define STRUCT_BASE		1000

#define STRUCT_PATTERN 	"[\n\t\r ]*struct[\n\t\r ]+([a-zA-Z0-9_]+)"	\
	"[\n\t\r ]*\\{[\n\t\r ]*([^}]*)\\}[\n\t\r ]*"			\
	"[a-zA-Z0-9_]*;[\n\t\r ]*"

#define ELEMENT_PATTERN	"[\n\t\r ]*([A-Za-z0-9_]+)[\n\t\r ]+" \
	"([a-zA-Z_][a-zA-Z0-9_]*)(\\[([0-9]+)\\])?[\n\t\r ]*;[\n\t\r ]*"

#define INI_PATTERN	"^[\n\t\r ]*([A-Za-z_][A-Za-z0-9_.]*)[\n\t\r ]*=" \
	"[\n\t\r ]*([A-Za-z0-9_]+)"

#define CC_COMMENT_PATTERN	"(([^/]|[^/][^/])*)//[^\n]*\n(.*)"

#define C_COMMENT_PATTERN	"(([^/]|[^/][^*])*)/\\*(([^*]|[^*][^/])*)\\*/(.*)"


/* we only match WL12XX and WL18XX magic */
#define DEFINE_PATTERN	"#define[\n\t\r ]+([A-Za-z_][A-Za-z0-9_]*)"	\
	"[\n\t\r ]+(0x[0-9A-Fa-f]+)"


static struct structure *structures = NULL;
static int n_structs = 0;

static int magic	= 0;
static int version	= 0;
static int checksum	= 0;

static int get_type(char *type_str)
{
	int i;

	for (i = 0; i < (sizeof(types) / sizeof(types[0])); i++)
		if (!strcmp(type_str, types[i].name))
			return i;

	for (i = 0; i < n_structs; i++) {
		struct structure *curr_struct;
		curr_struct = &structures[i];

		if (!strcmp(type_str, curr_struct->name)) {
			return STRUCT_BASE + i;
		}
	}

	return -1;
}

static char *remove_comments(const char *orig_str)
{
	regex_t r;
	char *new_str = NULL;
	regmatch_t m[6];

	if (regcomp(&r, CC_COMMENT_PATTERN, REG_EXTENDED) < 0)
		goto out;

	new_str = strdup(orig_str);

	while (!regexec(&r, new_str, 4, m, 0)) {
		char *part1, *part2;

		part1 = strndup(new_str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
		part2 = strndup(new_str + m[3].rm_so, m[3].rm_eo - m[3].rm_so);

		snprintf(new_str, strlen(orig_str), "%s%s", part1, part2);

		free(part1);
		free(part2);
	}

	regfree(&r);

	if (regcomp(&r, C_COMMENT_PATTERN, REG_EXTENDED) < 0)
		goto out;

	while (!regexec(&r, new_str, 6, m, 0)) {
		char *part1, *part2;

		part1 = strndup(new_str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
		part2 = strndup(new_str + m[5].rm_so, m[5].rm_eo - m[5].rm_so);

		snprintf(new_str, strlen(orig_str), "%s%s", part1, part2);

		free(part1);
		free(part2);
	}

	regfree(&r);
out:
	return new_str;
}

static struct structure *get_struct(const char *structure)
{
	int j;

	for (j = 0; j < n_structs; j++)
		if (!strcmp(structure, structures[j].name))
			return &structures[j];

	return NULL;
}

static int parse_define(const char *buffer, char *symbol, int *value)
{
	regex_t r;
	int ret;
	const char *str;

	ret = regcomp(&r, DEFINE_PATTERN, REG_EXTENDED);
	if (ret < 0)
		goto out;

	str = buffer;

	while (strlen(str)) {
		regmatch_t m[3];
		char *symbol_str, *value_str = NULL;

		if (regexec(&r, str, 3, m, 0))
			break;

		symbol_str =
			strndup(str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);

		value_str = strndup(str + m[2].rm_so, m[2].rm_eo - m[2].rm_so);

		if (!strcmp(symbol, symbol_str)) {
			if (*value == 0) {
				*value = strtol(value_str, NULL, 0);
				printf("symbol %s found %s (%08x)\n",
				       symbol, value_str, *value);
			} else {
				fprintf(stderr, "symbol %s redefined\n",
					symbol_str);
				ret = -1;
			}
		}

		free(symbol_str);
		symbol_str = NULL;
		free(value_str);
		value_str = NULL;

		str += m[2].rm_eo;
	};

out:
	regfree(&r);
	return ret;
}

static int parse_elements(char *orig_str, struct element **elements,
			  size_t *size)
{
	regex_t r;
	int ret, n_elements = 0;
	char *str, *clean_str;

	*size = 0;

	ret = regcomp(&r, ELEMENT_PATTERN, REG_EXTENDED);
	if (ret < 0)
		goto out;

	clean_str = remove_comments(orig_str);
	if (!clean_str)
		goto out;

	*elements = NULL;

	str = clean_str;

	while (strlen(str)) {
		regmatch_t m[6];
		char *type_str, *array_size_str = NULL;
		struct element *curr_element;

		if (regexec(&r, str, 6, m, 0))
			break;

		*elements = realloc(*elements,
				    ++n_elements * sizeof(**elements));
		if (!elements) {
			ret = -1;
			goto out_free;
		}

		curr_element = &(*elements)[n_elements - 1];

		curr_element->name =
			strndup(str + m[2].rm_so, m[2].rm_eo - m[2].rm_so);

		curr_element->position = *size;

		if (m[4].rm_so == m[4].rm_eo) {
			curr_element->array_size = 1;
		} else {
			array_size_str =
				strndup(str + m[4].rm_so, m[4].rm_eo - m[4].rm_so);
			curr_element->array_size = strtol(array_size_str, NULL, 0);
		}

		type_str = strndup(str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
		curr_element->type = get_type(type_str);
		if (curr_element->type == -1)
			fprintf(stderr, "Error! Unknown type '%s'\n", type_str);

		if (curr_element->type < STRUCT_BASE)
			*size += curr_element->array_size *
				types[curr_element->type].size;
		else {
			struct structure *structure =
				get_struct(type_str);

			*size += curr_element->array_size *
				structure->size;
		}

		free(type_str);
		free(array_size_str);

		str += m[2].rm_eo;
	}

	ret = n_elements;

out_free:
	free(clean_str);
	regfree(&r);
out:
	return ret;
}

static void print_usage(char *executable)
{
	printf("Usage:\n\t%s [OPTIONS] [COMMANDS]\n"
	       "\n\tOPTIONS\n"
	       "\t-S, --source-struct\tuse the structure specified in a C header file\n"
	       "\t-b, --binary-struct\tspecify the binary file where the structure is defined\n"
	       "\t-i, --input-config\tlocation of the input binary configuration file\n"
	       "\t-o, --output-config\tlocation of the input binary configuration file\n"
	       "\n\tCOMMANDS\n"
	       "\t-g, --get\t\tget the value of the specified element (element[.element...])\n"
	       "\t-s, --set\t\tset the value of the specified element (element[.element...])\n"
	       "\t-G, --generate-struct\tgenerate the binary structure file from\n"
	       "\t\t\t\tthe specified source file\n"
	       "\t-I, --parse-ini\t\tparse the specified INI file and set the values accordingly\n"
	       "\t\t\t\tin the output binary configuration file\n"
	       "\t-p, --print-struct\tprint out the structure\n"
	       "\t-d, --dump\t\tdump the entire configuration binary in human-readable format\n"
	       "\t-h, --help\t\tprint this help\n"
	       "\n",
	       executable);
}

static void free_structs(void)
{
	int i;

	for (i = 0; i < n_structs; i++)
		free(structures[i].elements);

	free(structures);
}

static int parse_header(const char *buffer)
{
	regex_t r;
	const char *str;
	int ret;

	ret = parse_define(buffer, DEFAULT_MAGIC_SYMBOL, &magic);
	if (ret < 0)
		goto out;

	ret = parse_define(buffer, DEFAULT_VERSION_SYMBOL, &version);
	if (ret < 0)
		goto out;

	ret = regcomp(&r, STRUCT_PATTERN, REG_EXTENDED);
	if (ret < 0)
		goto out;

	str = buffer;

	while (strlen(str)) {
		char *elements_str;
		struct structure *curr_struct;
		regmatch_t m[4];

		if (regexec(&r, str, 4, m, 0))
			break;

		structures = realloc(structures, ++n_structs *
				     sizeof(struct structure));
		if (!structures) {
			ret = -1;
			goto out_free;
		}

		curr_struct = &structures[n_structs - 1];

		curr_struct->name =
			strndup(str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);

		elements_str = strndup(str + m[2].rm_so, m[2].rm_eo - m[2].rm_so);

		ret = parse_elements(elements_str, &curr_struct->elements,
				     &curr_struct->size);
		if (ret < 0)
			break;

		curr_struct->n_elements = ret;

		str += m[2].rm_eo;
	}

out_free:
	regfree(&r);
out:
	return ret;
}

static size_t print_data(struct element *elem, void *data, int level)
{
	uint8_t *u8;
	uint16_t *u16;
	uint32_t *u32;
	int i;
	char *pos = data;
	char indent[MAX_INDENT];

	indent[0] = '\0';

	for (i = 0; i < level; i++)
		strncat(indent, INDENT_CHAR, sizeof(indent));

	switch (types[elem->type].size) {
	case 1:
		u8 = data;
		printf("0x%02x\n", *u8);
		break;
	case 2:
		u16 = data;
		printf("0x%04x\n", *u16);
		break;
	case 4:
		for (i = 0; i < elem->array_size; i++) {
			if ((elem->array_size > 1) && (i % 4 == 0))
				printf("\n%s", indent);
			u32 = (uint32_t *) pos;
			printf("0x%08x, ", *u32);
			pos += sizeof(uint32_t);
		}
		printf("\n");
		break;
	default:
		fprintf(stderr, "Error! Unsupported data size\n");
		break;
	}

	return types[elem->type].size * elem->array_size;
}

static int set_data(struct element *elem, void *buffer, void *data)
{
	switch (types[elem->type].size) {
	case 1:
		*((uint8_t *)buffer) = *((uint8_t *)data);
		break;
	case 2:
		*((uint16_t *)buffer) = *((uint16_t *)data);
		break;
	case 4:
		*((uint32_t *)buffer) = *((uint32_t *)data);
		break;
	default:
		fprintf(stderr, "Error! Unsupported data size\n");
		break;
	}

	return 0;
}

static size_t print_element(struct element *elem, int level, void *data)
{
	char indent[MAX_INDENT];
	char *pos = data;
	size_t len = 0;
	int i;

	if (level > MAX_INDENT) {
		fprintf(stderr, "Max indentation level exceeded!\n");
		level = MAX_INDENT;
	}

	indent[0] = '\0';

	for (i = 0; i < level; i++)
		strncat(indent, INDENT_CHAR, sizeof(indent));

	printf("%s%d\t", indent, elem->position);

	if (elem->type < STRUCT_BASE) {
		printf("%s %s[%d]",
		       types[elem->type].name,
		       elem->name,
		       elem->array_size);
		if (data) {
			if (elem->array_size) {
				printf(" = ");
				len += print_data(elem, pos, level + 1);
				pos += len;
			} else {
				printf("\n");
			}
		} else {
			printf(" (size = %d bytes)\n",
			       types[elem->type].size * elem->array_size);
		}
	} else {
		struct structure *sub;
		int j;

		sub = &structures[elem->type -
				  STRUCT_BASE];
		printf("struct %s %s (size = %d bytes)\n",
		       sub->name, elem->name, sub->size);

		for (j = 0; j < sub->n_elements; j++) {
			len += print_element(&sub->elements[j],
					     level + 1, pos);
			pos += len;
		}
	}

	return len;
}

static void print_structs(void *buffer, struct structure *structure)
{
	int i, len;
	char *pos = buffer;

	for (i = 0; i < structure->n_elements; i++) {
		len = print_element(&structure->elements[i],
				    1, pos);
		if (pos)
			pos += len;
	}
}

static int read_file(const char *filename, void **buffer, size_t size)
{
	FILE *file;
	struct stat st;
	int ret;
	size_t buf_size;

	ret = stat(filename, &st);
	if (ret < 0) {
		fprintf(stderr, "Couldn't get file size '%s'\n", filename);
		goto out;
	}

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Couldn't open file '%s'\n", filename);
		return -1;
	}

	if (size)
		buf_size = size;
	else
		buf_size = st.st_size;

	*buffer = malloc(buf_size);
	if (!*buffer) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n", buf_size);
		fclose(file);
		return -1;
	}

	if (fread(*buffer, 1, buf_size, file) != buf_size) {
		fprintf(stderr, "Failed to read file '%s'\n", filename);
		fclose(file);
		return -1;
	}

	fclose(file);
out:
	return ret;
}

static void free_file(void *buffer)
{
	free(buffer);
}

static int write_file(const char *filename, const void *buffer, size_t size)
{
	FILE *file;
	int ret;

	file = fopen(filename, "w");
	if (!file) {
		fprintf(stderr, "Couldn't open file '%s' for writing\n",
			filename);
		ret = -1;
		goto out;
	}

	if (fwrite(buffer, 1, size, file) != size) {
		fprintf(stderr, "Failed to write file '%s'\n", filename);
		fclose(file);
		ret = -1;
		goto out;
	}

	fclose(file);
out:
	return ret;
}

static int get_element_pos(struct structure *structure, const char *argument,
			   struct element **element)
{
	int i, pos = 0;
	struct structure *curr_struct = structure;
	struct element *curr_element = NULL;
	char *str, *arg = strdup(argument);

	str = strtok(arg, ".");

	while(str) {

		for (i = 0; i < curr_struct->n_elements; i++)
			if (!strcmp(str, curr_struct->elements[i].name)) {
				curr_element = &curr_struct->elements[i];
				pos += curr_element->position;
				break;
			}

		if (i == curr_struct->n_elements) {
			fprintf(stderr, "couldn't find element %s\n", str);
			pos = -1;
			goto out;
		}

		str = strtok(NULL, ".");
		if (str && curr_element->type < STRUCT_BASE) {
			fprintf(stderr, "element %s is not a struct\n",
				curr_element->name);
			pos = -1;
			goto out;
		}

		if (str)
			curr_struct =
				&structures[curr_element->type - STRUCT_BASE];
	}

out:
	*element = curr_element;
	free(arg);
	return pos;
}

static void get_value(void *buffer, struct structure *structure,
		      char *argument)
{
	int pos;
	struct element *element;

	pos = get_element_pos(structure, argument, &element);
	if (pos < 0) {
		fprintf(stderr, "couldn't find %s\n", argument);
		return;
	}

	if (element->type >= STRUCT_BASE) {
		fprintf(stderr,
			"getting entire structures not supported yet.\n");
		return;
	}

	printf("%s = ", argument);
	print_data(element, ((char *)buffer) + pos, 0);
}

static int set_value(void *buffer, struct structure *structure,
		     char *argument)
{
	int pos, ret = 0;
	char *split_point, *element_str, *value_str;
	struct element *element;
	long int value;

	split_point = strchr(argument, '=');
	if (!split_point) {
		fprintf(stderr,
			"--set requires the format <element>[.<element>...]=<value>");
		ret = -1;
		goto out;
	}

	*split_point = '\0';
	element_str = argument;
	value_str = split_point + 1;

	pos = get_element_pos(structure, element_str, &element);
	if (pos < 0) {
		fprintf(stderr, "couldn't find %s\n", element_str);
		ret = -1;
		goto out;
	}

	if (element->array_size > 1) {
		fprintf(stderr, "setting arrays not supported yet\n");
		ret = -1;
		goto out;
	}

	if (element->type >= STRUCT_BASE) {
		fprintf(stderr,
			"setting entire structures is not supported.\n");
		ret = -1;
		goto out;
	}

	value = strtol(value_str, NULL, 0);
	ret = set_data(element, ((char *)buffer) + pos, &value);

out:
	return ret;
}

#define WRITE_VAL(val, file) {				\
		fwrite(&val, 1, sizeof(val), file);	\
	}

#define READ_VAL(val, file) {				\
		fread(&val, 1, sizeof(val), file);	\
	}

static int write_element(FILE *file, struct element *element)
{
	size_t name_len = strlen(element->name);

	WRITE_VAL(name_len, file);
	fwrite(element->name, 1, name_len, file);
	WRITE_VAL(element->type, file);
	WRITE_VAL(element->array_size, file);
	WRITE_VAL(element->position, file);

	return 0;
}

static int write_struct(FILE *file, struct structure *structure)
{
	size_t name_len = strlen(structure->name);
	int i, ret = 0;

	WRITE_VAL(name_len, file);
	fwrite(structure->name, 1, name_len, file);
	WRITE_VAL(structure->n_elements, file);
	WRITE_VAL(structure->size, file);

	for (i = 0; i < structure->n_elements; i++) {
		ret = write_element(file, &structure->elements[i]);
		if (ret < 0)
			break;
	}

	return ret;
}

static int generate_struct(const char *filename)
{
	FILE *file;
	int i, ret = 0;

	file = fopen(filename, "w");
	if (!file) {
		fprintf(stderr, "Couldn't open file '%s' for writing\n",
			filename);
		ret = -1;
		goto out;
	}

	WRITE_VAL(magic, file);
	WRITE_VAL(version, file);
	WRITE_VAL(checksum, file);
	WRITE_VAL(n_structs, file);

	for (i = 0; i < n_structs; i++) {
		ret = write_struct(file, &structures[i]);
		if (ret < 0)
			break;
	}

	fclose(file);
out:
	return ret;
}

static int read_element(FILE *file, struct element *element)
{
	size_t name_len;
	int ret = 0;

	READ_VAL(name_len, file);

	element->name = malloc(name_len + 1);
	if (!element->name) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n",
			name_len);
		ret = -1;
		goto out;
	}

	fread(element->name, 1, name_len, file);
	element->name[name_len] = '\0';

	READ_VAL(element->type, file);
	READ_VAL(element->array_size, file);
	READ_VAL(element->position, file);

out:
	return ret;
}

static int read_struct(FILE *file, struct structure *structure)
{
	size_t name_len;
	int i, ret = 0;

	READ_VAL(name_len, file);

	structure->name = malloc(name_len + 1);
	if (!structure->name) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n",
			name_len);
		ret = -1;
		goto out;
	}

	fread(structure->name, 1, name_len, file);
	structure->name[name_len] = '\0';

	READ_VAL(structure->n_elements, file);
	READ_VAL(structure->size, file);

	structure->elements = malloc(structure->n_elements *
				     sizeof(struct element));
	if (!structure->elements) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n",
			structure->n_elements * sizeof(struct element));
		ret = -1;
		goto out;
	}

	for (i = 0; i < structure->n_elements; i++) {
		ret = read_element(file, &structure->elements[i]);
		if (ret < 0)
			break;
	}

out:
	return ret;
}

static int read_binary_struct(const char *filename)
{
	FILE *file;
	int i, ret = 0;

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Couldn't open file '%s' for reading\n",
			filename);
		ret = -1;
		goto out;
	}

	READ_VAL(magic, file);
	READ_VAL(version, file);
	READ_VAL(checksum, file);
	READ_VAL(n_structs, file);

	structures = malloc(n_structs * sizeof(struct structure));
	if (!structures) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n",
			n_structs * sizeof(struct structure));
		ret = -1;
		goto out_close;
	}

	for (i = 0; i < n_structs; i++) {
		ret = read_struct(file, &structures[i]);
		if (ret < 0)
			break;
	}

out_close:
	fclose(file);
out:
	return ret;
}

static int get_value_int(void *buffer, struct structure *structure,
			 int *value, char *element_str)
{
	int pos, ret = 0;
	struct element *element;

	pos = get_element_pos(structure, element_str, &element);
	if (pos < 0) {
		fprintf(stderr, "couldn't find %s\n", element_str);
		ret = pos;
		goto out;
	}

	if ((element->type != get_type("u32")) &&
	    (element->type != get_type("__le32"))) {
		fprintf(stderr,
			"element %s has invalid type (expected u32 or le32)\n",
			element_str);
		ret = -1;
		goto out;
	}

	*value = *(int *) (((char *)buffer) + pos);

out:
	return ret;
}

static int read_input(const char *filename, void **buffer,
		      struct structure *structure)
{
	int ret;
	int input_magic, input_version, input_checksum;

	ret = read_file(filename, buffer, structure->size);
	if (ret < 0)
		goto out;

	ret = get_value_int(*buffer, structure, &input_magic,
			    DEFAULT_MAGIC_ELEMENT);
	if (ret < 0)
		goto out;

	ret = get_value_int(*buffer, structure, &input_version,
			    DEFAULT_VERSION_ELEMENT);
	if (ret < 0)
		goto out;

	ret = get_value_int(*buffer, structure, &input_checksum,
			    DEFAULT_CHKSUM_ELEMENT);
	if (ret < 0)
		goto out;

	if ((magic != input_magic) ||
	    (version != input_version) ||
	    (checksum != input_checksum)) {
		fprintf(stderr, "incompatible binary file\n");
		fprintf(stderr,
			"expected 0x%08x 0x%08x 0x%08x\n"
			"got 0x%08x 0x%08x 0x%08x\n",
			magic, version, checksum,
			input_magic, input_version, input_checksum);


		ret = -1;
		goto out;
	}

out:
	return ret;
}

static int parse_ini(void *conf_buffer, struct structure *structure,
		     const char *buffer)
{
	regex_t r;
	const char *str;
	int ret;

	ret = regcomp(&r, INI_PATTERN, REG_EXTENDED);
	if (ret < 0)
		goto out;

	str = buffer;

	while (strlen(str)) {
		char *element_str = NULL, *value_str = NULL;
		regmatch_t m[3];
		struct element *element;
		long int value;
		int pos;

		if (regexec(&r, str, 3, m, 0))
			break;

		element_str =
			strndup(str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);

		value_str = strndup(str + m[2].rm_so, m[2].rm_eo - m[2].rm_so);

		pos = get_element_pos(structure, element_str, &element);
		if (pos < 0) {
			fprintf(stderr, "couldn't find %s\n", element_str);
			ret = -1;
		} else if (element->array_size > 1) {
			fprintf(stderr, "setting arrays not supported yet\n");
			ret = -1;
		} else if (element->type >= STRUCT_BASE) {
			fprintf(stderr,
				"setting entire structures is not supported.\n");
			ret = -1;
		} else {
			value = strtol(value_str, NULL, 0);
			ret = set_data(element, ((char *)conf_buffer) + pos, &value);
		}

		free(element_str);
		element_str = NULL;
		free(value_str);
		value_str = NULL;

		str += m[2].rm_eo;
	}

	regfree(&r);
out:
	return ret;
}

#define SHORT_OPTIONS "S:s:b:i:o:g:G:I:pdh"

struct option long_options[] = {
	{ "binary-struct",	required_argument,	NULL,	'b' },
	{ "source-struct",	required_argument,	NULL,	'S' },
	{ "input-config",	required_argument,	NULL,	'i' },
	{ "output-config",	required_argument,	NULL,	'o' },
	{ "get",		required_argument,	NULL,	'g' },
	{ "set",		required_argument,	NULL,	's' },
	{ "generate-struct",	required_argument,	NULL,	'G' },
	{ "parse-ini",		required_argument,	NULL,	'I' },
	{ "print-struct",	no_argument,		NULL,	'p' },
	{ "dump",		no_argument,		NULL,	'd' },
	{ "help",		no_argument,		NULL,	'h' },
	{ 0, 0, 0, 0 },
};

int main(int argc, char **argv)
{
	void *header_buf = NULL;
	void *conf_buf = NULL;
	void *ini_buf = NULL;
	char *header_filename = NULL;
	char *binary_struct_filename = NULL;
	char *input_filename = NULL;
	char *output_filename = NULL;
	char *command_arg = NULL;
	struct structure *root_struct;
	int c, ret = 0;
	char command = 0;

	while (1) {
		c = getopt_long(argc, argv, SHORT_OPTIONS, long_options, NULL);

		if (c < 0)
			break;

		switch(c) {
		case 'S':
			header_filename = optarg;
			break;

		case 'b':
			binary_struct_filename = strdup(optarg);
			break;

		case 'i':
			input_filename = strdup(optarg);
			break;

		case 'o':
			output_filename = strdup(optarg);
			break;

		case 'G':
		case 'g':
		case 's':
		case 'I':
			command_arg = optarg;
			/* Fall through */
		case 'p':
		case 'd':
			if (command) {
				fprintf(stderr,
					"Only one command option is allowed, can't use -%c with -%c.\n",
					command, c);
				print_usage(argv[0]);
				exit(-1);
			}
			command = c;
			break;

		case 'h':
			print_usage(argv[0]);
			exit(0);

		default:
			print_usage(argv[0]);
			exit(-1);
		}
	}

	if (!input_filename)
		input_filename = strdup(DEFAULT_INPUT_FILENAME);

	if (!output_filename)
		output_filename = strdup(DEFAULT_OUTPUT_FILENAME);

	if (header_filename && binary_struct_filename) {
		fprintf(stderr,
			"Can't specify both source struct and binary struct\n");
		ret = -1;
		goto out;
	}

	if (!header_filename && !binary_struct_filename) {
		binary_struct_filename = strdup(DEFAULT_BIN_FILENAME);
		ret = read_binary_struct(binary_struct_filename);
		if (ret < 0)
			goto out;
	}

	if (header_filename) {
		ret = read_file(header_filename, &header_buf, 0);
		if (ret < 0)
			goto out;

		ret = parse_header(header_buf);
		if (ret < 0)
			goto out;
	}

	root_struct = get_struct(DEFAULT_ROOT_STRUCT);
	if (!root_struct) {
		fprintf(stderr,
			"error: root struct (%s) is not defined\n",
			DEFAULT_ROOT_STRUCT);
		ret = -1;
		goto out;
	}

	switch (command) {
	case 'G':
		if (!header_buf) {
			fprintf(stderr, "Source struct file must be specified.\n");
			ret = -1;
			break;
		}

		ret = generate_struct(command_arg);
		break;

	case 'g':
		ret = read_input(input_filename, &conf_buf, root_struct);
		if (ret < 0)
			goto out;

		get_value(conf_buf, root_struct, command_arg);
		break;

	case 's':
		ret = read_input(input_filename, &conf_buf, root_struct);
		if (ret < 0)
			goto out;

		ret = set_value(conf_buf, root_struct, command_arg);
		if (ret < 0)
			goto out;

		ret = write_file(output_filename, conf_buf, root_struct->size);
		if (ret < 0)
			goto out;

		break;

	case 'I':
		ret = read_input(input_filename, &conf_buf, root_struct);
		if (ret < 0)
			goto out;

		ret = read_file(command_arg, &ini_buf, 0);
		if (ret < 0)
			goto out;

		ret = parse_ini(conf_buf, root_struct, ini_buf);
		if (ret < 0)
			goto out;

		ret = write_file(output_filename, conf_buf, root_struct->size);
		if (ret < 0)
			goto out;

		break;

	case 'p':
		print_structs(NULL, root_struct);
		break;

	case 'd':
		/* fall through -- dump is the default if not specified */
	default:
		ret = read_input(input_filename, &conf_buf, root_struct);
		if (ret < 0)
			goto out;

		print_structs(conf_buf, root_struct);
		break;
	}

	free_file(header_buf);
	free_file(conf_buf);

	free_structs();
out:
	free(input_filename);
	free(binary_struct_filename);

	exit(ret);
}
