/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**********************************************************
*      This software is part of the graphviz package      *
*                http://www.graphviz.org/                 *
*                                                         *
*            Copyright (c) 1994-2004 AT&T Corp.           *
*                and is licensed under the                *
*            Common Public License, Version 1.0           *
*                      by AT&T Corp.                      *
*                                                         *
*        Information and Software Systems Research        *
*              AT&T Research, Florham Park NJ             *
**********************************************************/


/*
 *  Code for main functions in gpr
 */

#include <actions.h>
#include <error.h>
#include <ast.h>
#include "compile.h"
#include "sfstr.h"
#include <string.h>
#include <stdio.h>

#define KINDS(p) ((AGTYPE(p) == AGRAPH) ? "graph" : (AGTYPE(p) == AGNODE) ? "node" : "edge")

/* sameG:
 * Return common root if objects belong to same root graph.
 * NULL otherwise
 */
Agraph_t *sameG(void *p1, void *p2, char *fn, char *msg)
{
    Agobj_t *obj1 = OBJ(p1);
    Agobj_t *obj2 = OBJ(p2);
    Agraph_t *root;

    root = agroot(agraphof(obj1));
    if (root != agroot(agraphof(obj2))) {
	if (msg)
	    error(ERROR_WARNING, "%s in %s() belong to different graphs",
		  msg, fn);
	else
	    error(ERROR_WARNING,
		  "%s and %s in %s() belong to different graphs",
		  KINDS(obj1), KINDS(obj2), fn);
	return 0;
    } else
	return root;
}

/* indexOf:
 * Return index of leftmost string s2 in string s1, or -1
 */
int indexOf(char *s1, char *s2)
{
    char c1 = *s2;
    char c;
    char *p;
    int len2;

    if (c1 == '\0')
	return 0;
    p = s1;
    len2 = strlen(s2) - 1;
    while ((c = *p++)) {
	if (c != c1)
	    continue;
	if (strncmp(p, s2 + 1, len2) == 0)
	    return ((p - s1) - 1);
    }
    return -1;
}

/* rindexOf:
 * Return index of rightmost string s2 in string s1, or -1
 */
int rindexOf(char *s1, char *s2)
{
    char c1 = *s2;
    char c;
    char *p;
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    if (c1 == '\0')
	return (len1);
    p = s1 + (len1 - len2);
    while (p >= s1) {
	c = *p;
	if ((c == c1) && (strncmp(p+1, s2+1, len2-1) == 0))
	    return (p - s1);
	else
	    p--;
    }
    return -1;
}

/* match:
 * Return index of pattern pat in string str, or -1
 */
int match(char *str, char *pat)
{
    int sub[2];

    if (strgrpmatch(str, pat, sub, 1, STR_MAXIMAL)) {
	return (sub[0]);
    } else
	return -1;
}

/* nodeInduce:
 * Add all edges in root graph connecting two nodes in 
 * selected to selected.
 */
void nodeInduce(Agraph_t * selected)
{
    Agnode_t *n;
    Agedge_t *e;
    Agraph_t *base;

    if (!selected)
	return;
    base = agroot(selected);
    if (base == selected)
	return;
    for (n = agfstnode(selected); n; n = agnxtnode(selected, n)) {
	for (e = agfstout(base, n); e; e = agnxtout(base, e)) {
	    if (agsubnode(selected, aghead(e), FALSE))
		agsubedge(selected, e, TRUE);
	}
    }
}

/* copyAttr;
 * Copy attributes from src to tgt. Overrides currently
 * defined values.
 * FIX: we should probably use the default value of the source
 * graph when initializing the attribute, rather than "".
 * NOTE: We do not assume src and tgt have the same kind.
 */
int copyAttr(Agobj_t * src, Agobj_t * tgt)
{
    Agraph_t *srcg;
    Agraph_t *tgtg;
    Agsym_t *sym = 0;
    Agsym_t *tsym = 0;
    int skind = AGTYPE(src);
    int tkind = AGTYPE(tgt);
    char* val;

    srcg = agraphof(src);
    tgtg = agraphof(tgt);
    while ((sym = agnxtattr(srcg, skind, sym))) {
	tsym = agattrsym(tgt, sym->name);
	if (!tsym)
	    tsym = agattr(tgtg, tkind, sym->name, sym->defval);
	val = agxget(src, sym);
	if (aghtmlstr (val)) {
	    val = agstrdup_html (tgtg, val);
	    agxset(tgt, tsym, val);
	    agstrfree (tgtg, val);
	}
	else
	    agxset(tgt, tsym, val);
    }
    return 0;
}

/* copy:
 * Create new object of type AGTYPE(obj) with all of its
 * attributes.
 * If obj is an edge, only create end nodes if necessary.
 * If obj is a graph, if g is null, create a top-level
 * graph. Otherwise, create a subgraph of g.
 * Assume obj != NULL.
 */
Agobj_t *copy(Agraph_t * g, Agobj_t * obj)
{
    Agobj_t *nobj = 0;
    Agedge_t *e;
    Agnode_t *h;
    Agnode_t *t;
    int kind = AGTYPE(obj);
    char *name = agnameof(obj);

    if ((kind != AGRAPH) && !g) {
	error(ERROR_FATAL, "NULL graph with non-graph object in copy()");
	return 0;
    }

    switch (kind) {
    case AGNODE:
	nobj = (Agobj_t *) openNode(g, name);
	break;
    case AGRAPH:
	if (g)
	    nobj = (Agobj_t *) openSubg(g, name);
	else
	    nobj = (Agobj_t *) openG(name, ((Agraph_t *) obj)->desc);
	break;
    case AGEDGE:
	e = (Agedge_t *) obj;
	t = openNode(g, agnameof(agtail(e)));
	h = openNode(g, agnameof(aghead(e)));
	nobj = (Agobj_t *) openEdge(g, t, h, name);
	break;
    }
    if (nobj)
	copyAttr(obj, nobj);

    return nobj;
}

typedef struct {
    Dtlink_t link;
    Agedge_t *key;
    Agedge_t *val;
} edgepair_t;

static Agedge_t*
mapEdge (Dt_t* emap, Agedge_t* e)
{
     edgepair_t* ep = dtmatch (emap, &e);     
     if (ep) return ep->val;
     else return NULL;
}

/* cloneSubg:
 * Clone subgraph sg in tgt.
 */
static Agraph_t *cloneSubg(Agraph_t * tgt, Agraph_t * g, Dt_t* emap)
{
    Agraph_t *ng;
    Agraph_t *sg;
    Agnode_t *t;
    Agnode_t *newt;
    Agedge_t *e;
    Agedge_t *newe;

    ng = (Agraph_t *) (copy(tgt, OBJ(g)));
    if (!ng)
	return 0;
    for (t = agfstnode(g); t; t = agnxtnode(g, t)) {
	newt = agnode(tgt, agnameof(t), 0);
	if (!newt)
	    error(ERROR_PANIC, "node %s not found in cloned graph %s",
		  agnameof(t), agnameof(tgt));
	agsubnode(ng, newt, 1);
    }
    for (t = agfstnode(g); t; t = agnxtnode(g, t)) {
	for (e = agfstout(g, t); e; e = agnxtout(g, e)) {
	    newe = mapEdge (emap, e);
	    if (!newe)
		error(ERROR_PANIC,
		      "edge (%s,%s)[%s] not found in cloned graph %s",
		      agnameof(agtail(e)), agnameof(aghead(e)),
		      agnameof(e), agnameof(tgt));
	    agsubedge(ng, newe, 1);
	}
    }
    for (sg = agfstsubg(g); sg; sg = agnxtsubg(sg)) {
	if (!cloneSubg(ng, sg, emap)) {
	    error(ERROR_FATAL, "error cloning subgraph %s from graph %s",
		  agnameof(sg), agnameof(g));
	}
    }
    return ng;
}

static int cmppair(Dt_t * d, Agedge_t** key1, Agedge_t** key2, Dtdisc_t * disc)
{
    if (*key1 > *key2) return 1;
    else if (*key1 < *key2) return -1;
    else return 0;
}

static Dtdisc_t edgepair = {
    offsetof(edgepair_t, key),
    sizeof(Agedge_t*),
    offsetof(edgepair_t, link),
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    (Dtcompar_f) cmppair,
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

/* cloneGraph:
 * Clone node, edge and subgraph structure from src to tgt.
 */
static void cloneGraph(Agraph_t * tgt, Agraph_t * src)
{
    Agedge_t *e;
    Agedge_t *ne;
    Agnode_t *t;
    Agraph_t *sg;
    Dt_t* emap = dtopen (&edgepair, Dtoset);
    edgepair_t* data = (edgepair_t*)malloc(sizeof(edgepair_t)*agnedges(src));
    edgepair_t* ep = data;

    for (t = agfstnode(src); t; t = agnxtnode(src, t)) {
	if (!copy(tgt, OBJ(t))) {
	    error(ERROR_FATAL, "error cloning node %s from graph %s",
		  agnameof(t), agnameof(src));
	}
    }
    for (t = agfstnode(src); t; t = agnxtnode(src, t)) {
	for (e = agfstout(src, t); e; e = agnxtout(src, e)) {
	    if (!(ne = (Agedge_t*)copy(tgt, OBJ(e)))) {
		error(ERROR_FATAL,
		      "error cloning edge (%s,%s)[%s] from graph %s",
		      agnameof(agtail(e)), agnameof(aghead(e)),
		      agnameof(e), agnameof(src));
	    }
	    ep->key = e;
	    ep->val = ne;
	    dtinsert (emap, ep++);
	}
    }
    for (sg = agfstsubg(src); sg; sg = agnxtsubg(sg)) {
	if (!cloneSubg(tgt, sg, emap)) {
	    error(ERROR_FATAL, "error cloning subgraph %s from graph %s",
		  agnameof(sg), agnameof(src));
	}
    }

    dtclose (emap);
    free (data);
}

/* clone:
 * Create new object of type AGTYPE(obj) with all of its
 * attributes and substructure.
 * If obj is an edge, end nodes are cloned if necessary.
 * If obj is a graph, if g is null, create a clone top-level
 * graph. Otherwise, create a clone subgraph of g.
 * Assume obj != NULL.
 */
Agobj_t *clone(Agraph_t * g, Agobj_t * obj)
{
    Agobj_t *nobj = 0;
    Agedge_t *e;
    Agnode_t *h;
    Agnode_t *t;
    int kind = AGTYPE(obj);
    char *name = agnameof(obj);

    if ((kind != AGRAPH) && !g) {
	error(ERROR_FATAL, "NULL graph with non-graph object in clone()");
	return 0;
    }

    switch (kind) {
    case AGNODE:		/* same as copy node */
	nobj = (Agobj_t *) openNode(g, name);
	if (nobj)
	    copyAttr(obj, nobj);
	break;
    case AGRAPH:
	if (g)
	    nobj = (Agobj_t *) openSubg(g, name);
	else
	    nobj = (Agobj_t *) openG(name, ((Agraph_t *) obj)->desc);
	if (nobj)
	    copyAttr(obj, nobj);
	cloneGraph((Agraph_t *) nobj, (Agraph_t *) obj);
	break;
    case AGEDGE:
	e = (Agedge_t *) obj;
	t = (Agnode_t *) clone(g, OBJ(agtail(e)));
	h = (Agnode_t *) clone(g, OBJ(aghead(e)));
	nobj = (Agobj_t *) openEdge(g, t, h, name);
	if (nobj)
	    copyAttr(obj, nobj);
	break;
    }

    return nobj;
}

#define CCMARKED(n)  (((nData(n))->iu.integer)&2)
#define CCMARK(n)    (((nData(n))->iu.integer) |= 2)
#define CCUNMARK(n)  (((nData(n))->iu.integer) &= ~2)

static void cc_dfs(Agraph_t* g, Agraph_t * comp, Agnode_t * n)
{
    Agedge_t *e;
    Agnode_t *other;

    CCMARK(n);
    agidnode(comp, AGID(n), 1);
    for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	if (agtail(e) == n)
	    other = aghead(e);
	else
	    other = agtail(e);
	if (!CCMARKED(other))
	    cc_dfs(g, comp, other);
    }
}

/* compOf:
 * Return connected component of node.
 */
Agraph_t *compOf(Agraph_t * g, Agnode_t * n)
{
    Agraph_t *cg;
    Agnode_t *np;
    static int id;
    char name[64];

    if (!(n = agidnode(g, AGID(n), 0)))
	return 0;		/* n not in g */
    for (np = agfstnode(g); np; np = agnxtnode(g, np))
	CCUNMARK(np);

    sprintf(name, "_cc_%d", id++);
    cg = openSubg(g, name);
    cc_dfs(g, cg, n);

    return cg;
}

/* isEdge:
 * Return edge, if any, between t and h with given key.
 * Edge is in g.
 */
Agedge_t *isEdge(Agraph_t* g, Agnode_t * t, Agnode_t * h, char *key)
{
    Agraph_t *root;

    root = sameG(t, h, "isEdge", "tail and head node");
    if (!root)
	return 0;
    if (g) {
	if (root != agroot(g)) return 0;
    }
    else
	g = root;

    return agedge(g, t, h, key, 0);
}

/* addNode:
 * Insert node n into subgraph g.
 * Return image of n
 */
Agnode_t *addNode(Agraph_t * gp, Agnode_t * np, int doAdd)
{
    if (!sameG(gp, np, "addNode", 0))
	return 0;
    return agsubnode(gp, np, doAdd);
}

/* addEdge:
 * Insert edge e into subgraph g.
 * Return image of e
 */
Agedge_t *addEdge(Agraph_t * gp, Agedge_t * ep, int doAdd)
{
    if (!sameG(gp, ep, "addEdge", 0))
	return 0;
    return agsubedge(gp, ep, doAdd);
}

/* lockGraph:
 * Set lock so that graph g will not be deleted.
 * g must be a root graph.
 * If v > 0, set lock
 * If v = 0, unset lock and delete graph is necessary.
 * If v < 0, no op
 * Always return previous lock state.
 * Return -1 on error.
 */
int lockGraph(Agraph_t * g, int v)
{
    gdata *data;
    int oldv;

    if (g != agroot(g)) {
	error(ERROR_WARNING,
	      "Graph argument to lock() is not a root graph");
	return -1;
    }
    data = gData(g);
    oldv = data->lock & 1;
    if (v > 0)
	data->lock |= 1;
    else if ((v == 0) && oldv) {
	if (data->lock & 2)
	    agclose(g);
	else
	    data->lock = 0;
    }
    return oldv;
}

/* deleteObj:
 * Remove obj from g.
 * obj may belong to a subgraph of g, so we first must map
 * obj to its version in g.
 * If g is null, remove object from root graph.
 * If obj is a (sub)graph, close it. The g parameter is unused.
 * Return 0 on success, non-zero on failure.
 */
int deleteObj(Agraph_t * g, Agobj_t * obj)
{
    gdata *data;
    if (AGTYPE(obj) == AGRAPH) {
	g = (Agraph_t *) obj;
	if (g != agroot(g))
	    return agclose(g);
	data = gData(g);
	if (data->lock & 1) {
	    error(ERROR_WARNING, "Cannot delete locked graph %s",
		  agnameof(g));
	    data->lock |= 2;
	    return -1;
	} else
	    agclose(g);
    }

    /* node or edge */
    if (!g)
	g = agroot(agraphof(obj));
    if (obj)
	return agdelete(g, obj);
    else
	return -1;
}

/* writeFile:
 * Write graph into file f.
 * Return 0 on success
 */
int writeFile(Agraph_t * g, char *f)
{
    int rv;
    Sfio_t *fp;

    if (!f) {
	error(ERROR_FATAL, "NULL string passed to writeG");
	return 1;
    }
    fp = sfopen(0, f, "w");
    if (!fp) {
	error(ERROR_FATAL, "Could not open %s for writing in writeG", f);
	return 1;
    }
    rv = agwrite(g, fp);
    sfclose(fp);
    return rv;
}

/* readFile:
 * Read graph from file f.
 * Return 0 on failure
 */
Agraph_t *readFile(char *f)
{
    Agraph_t *gp;
    Sfio_t *fp;

    if (!f) {
	error(ERROR_FATAL, "NULL string passed to readG");
	return 0;
    }
    fp = sfopen(0, f, "r");
    if (!fp) {
	error(ERROR_FATAL, "Could not open %s for reading in readG", f);
	return 0;
    }
    gp = readG(fp);
    sfclose(fp);

    return gp;
}

int fwriteFile(Expr_t * ex, Agraph_t * g, int fd)
{
    Sfio_t *sp;

    if (fd < 0 || fd >= elementsof(ex->file)
	|| !((sp = ex->file[fd]))) {
	exerror("fwriteG: %d: invalid descriptor", fd);
	return 0;
    }
    return agwrite(g, sp);
}

Agraph_t *freadFile(Expr_t * ex, int fd)
{
    Sfio_t *sp;

    if (fd < 0 || fd >= elementsof(ex->file)
	|| !((sp = ex->file[fd]))) {
	exerror("freadG: %d: invalid descriptor", fd);
	return 0;
    }
    return readG(sp);
}

int openFile(Expr_t * ex, char *fname, char *mode)
{
    int idx;

    /* find open index */
    for (idx = 3; idx < elementsof(ex->file); idx++)
	if (!ex->file[idx])
	    break;
    if (idx == elementsof(ex->file)) {
	error(ERROR_FATAL, "openF: no available descriptors");
	return -1;
    }
    ex->file[idx] = sfopen(0, fname, mode);
    if (ex->file[idx])
	return idx;
    else
	return -1;
}

int closeFile(Expr_t * ex, int fd)
{
    int rv;

    if ((0 <= fd) && (fd <= 2)) {
	error(ERROR_FATAL, "closeF: cannot close standard stream %d", fd);
	return -1;
    }
    if (!ex->file[fd]) {
	error(ERROR_FATAL, "closeF: stream %d not open", fd);
	return -1;
    }
    rv = sfclose(ex->file[fd]);
    if (!rv)
	ex->file[fd] = 0;
    return rv;
}

/*
 * Read single line from stream.
 * Return "" on EOF.
 */
char *readLine(Expr_t * ex, int fd)
{
    Sfio_t *sp;
    int c;
    Sfio_t *tmps;
    char *line;

    if (fd < 0 || fd >= elementsof(ex->file) || !((sp = ex->file[fd]))) {
	exerror("readL: %d: invalid descriptor", fd);
	return exstring(ex, "");
    }
    tmps = sfstropen();
    while (((c = sfgetc(sp)) > 0) && (c != '\n'))
	sfputc(tmps, c);
    if (c == '\n')
	sfputc(tmps, c);
    line = exstring(ex, sfstruse(tmps));
    sfclose(tmps);
    return line;
}

/* compare:
 * Lexicographic ordering of objects.
 */
int compare(Agobj_t * l, Agobj_t * r)
{
    char lkind, rkind;
    if (l == NULL) {
	if (r == NULL)
	    return 0;
	else
	    return -1;
    } else if (r == NULL) {
	return 1;
    }
    if (AGID(l) < AGID(r))
	return -1;
    else if (AGID(l) > AGID(r))
	return 1;
    lkind = AGTYPE(l);
    rkind = AGTYPE(r);
    if (lkind == 3)
	lkind = 2;
    if (rkind == 3)
	rkind = 2;
    if (lkind == rkind)
	return 0;
    else if (lkind < rkind)
	return -1;
    else
	return 1;
}

/* canon:
 * Canonicalize a string for printing.
 */
char *canon(Expr_t * pgm, char *arg)
{
    char *p;

    p = agcanonStr(arg);
    if (p != arg)
	p = exstring(pgm, p);

    return p;
}
