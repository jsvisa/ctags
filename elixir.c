/*
*   $Id: elixir.c 443 2014-03-05 04:37:13Z darren $
*
*   Copyright (c) 2014, jsvisa <delweng@gmail.com>
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for generating tags for Elixir language
*   files.  Some of the parsing constructs are based on the Emacs 'etags'
*   program by Francesco Potori <pot@gnu.org>
*/

/*
*   INCLUDE FILES
*/
#include "general.h"  /* must always come first */

#include <string.h>

#include "entry.h"
#include "options.h"
#include "read.h"
#include "routines.h"
#include "vstring.h"

/*
*   DATA DEFINITIONS
*/
typedef enum {
    K_MACRO, K_FUNCTION, K_MODULE, K_RECORD
} elixirKind;

static kindOption ElixirKinds[] = {
    {TRUE, 'd', "macro",    "macro definitions"},
    {TRUE, 'f', "function", "functions"},
    {TRUE, 'm', "module",   "modules"},
    {TRUE, 'r', "record",   "record definitions"},
};

/*
*   FUNCTION DEFINITIONS
*/
/* tagEntryInfo and vString should be preinitialized/preallocated but not
 * necessary. If successful you will find class name in vString
 */

static boolean isIdentifierFirstCharacter (int c)
{
    return (boolean) (isalpha (c));
}

static boolean isIdentifierCharacter (int c)
{
    return (boolean) (isalnum (c) || c == '_' || c == '.');
}

static const unsigned char *parseIdentifier (
        const unsigned char *cp, vString *const identifier)
{
    vStringClear (identifier);
    while (isIdentifierCharacter ((int) *cp))
    {
        vStringPut (identifier, (int) *cp);
        ++cp;
    }
    vStringTerminate (identifier);
    return cp;
}

static void skipWhitespace (const unsigned char** cp)
{
    while (isspace (**cp))
    {
        ++*cp;
    }
}

static void makeMemberTag (
        vString *const identifier, elixirKind kind, vString *const module)
{
    if (ElixirKinds [kind].enabled  &&  vStringLength (identifier) > 0)
    {
        tagEntryInfo tag;
        initTagEntry (&tag, vStringValue (identifier));
        tag.kindName = ElixirKinds[kind].name;
        tag.kind = ElixirKinds[kind].letter;

        if (module != NULL  &&  vStringLength (module) > 0)
        {
            tag.extensionFields.scope [0] = "module";
            tag.extensionFields.scope [1] = vStringValue (module);
        }
        makeTagEntry (&tag);
    }
}

static void parseModuleTag (const unsigned char *cp, vString *const module)
{
    vString *const identifier = vStringNew ();
    parseIdentifier (cp, identifier);
    makeSimpleTag (identifier, ElixirKinds, K_MODULE);

    /* All further entries go in the new module */
    vStringCopy (module, identifier);
    vStringDelete (identifier);
}

static void parseSimpleTag (const unsigned char *cp, elixirKind kind)
{
    vString *const identifier = vStringNew ();
    parseIdentifier (cp, identifier);
    makeSimpleTag (identifier, ElixirKinds, kind);
    vStringDelete (identifier);
}

static void parseFunctionTag (const unsigned char *cp, vString *const module)
{
    vString *const identifier = vStringNew ();
    parseIdentifier (cp, identifier);
    makeMemberTag (identifier, K_FUNCTION, module);
    vStringDelete (identifier);
}

/*
 * Directives are of the form:
 * def foo
 * defp foo
 * defmodule
 * defrecord
 */
static void parseDirective (const unsigned char *cp, vString *const module)
{
    /*
     * A directive will be either a record definition or a directive.
     * Record definitions are handled separately
     */
    vString *const directive = vStringNew ();
    const char *const drtv = vStringValue (directive);
    cp = parseIdentifier (cp, directive);
    skipWhitespace (&cp);
    /* if (*cp == '(') */
    /*     ++cp; */

    if (strcmp (drtv, "def") == 0 || strcmp (drtv, "defp") == 0)
        parseSimpleTag (cp, K_FUNCTION);
    else if (strcmp (drtv, "defmacro") == 0)
        parseSimpleTag (cp, K_MACRO);
    else if (strcmp (drtv, "defrecord") == 0)
        parseSimpleTag (cp, K_RECORD);
    else if (strcmp (drtv, "defmodule") == 0)
        parseModuleTag (cp, module);
    /* Otherwise, it was an import, require, etc. */

    vStringDelete (directive);
}

static void findElixirTags (void)
{
    vString *const module = vStringNew ();
    const unsigned char *line;

    while ((line = fileReadLine ()) != NULL)
    {
        const unsigned char *cp = line;

        skipWhitespace(&cp);

        if (*cp == '#')  /* skip initial comment */
            continue;
        if (*cp == '@')  /* strings sometimes start in column one */
            continue;

        if (*cp == 'd')
        {
            parseDirective(cp, module);
        }
        /* else if (isIdentifierFirstCharacter ((int) *cp)) */
        /*     parseFunctionTag (cp, module); */
    }
    vStringDelete (module);
}

extern parserDefinition *ElixirParser (void)
{
    static const char *const extensions[] = { "ex", "exs", NULL };
    parserDefinition *def = parserNew ("Elixir");
    def->kinds = ElixirKinds;
    def->kindCount = KIND_COUNT (ElixirKinds);
    def->extensions = extensions;
    def->parser = findElixirTags;
    return def;
}

/* vi:set tabstop=4 shiftwidth=4: */
