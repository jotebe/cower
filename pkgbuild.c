/*
 *  depends.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "aur.h"
#include "conf.h"
#include "pkgbuild.h"
#include "download.h"
#include "query.h"
#include "util.h"

static alpm_list_t *parse_bash_array(alpm_list_t *deplist, char **deparray, int stripver) {
  char *token;

  /* XXX: This will fail sooner or later */
  if (strchr(*deparray, ':')) { /* we're dealing with optdepdends */
    token = strtok(*deparray, "\'\"\n");
    while (token) {
      ltrim(token);
      if (strlen(token)) {
        if (config->verbose >= 2)
          fprintf(stderr, "::DEBUG Adding Depend: %s\n", token);
        deplist = alpm_list_add(deplist, strdup(token));
      }

      token = strtok(NULL, "\'\"\n");
    }
    return deplist;
  }

  token = strtok(*deparray, " \n");
  while (token) {
    ltrim(token);
    if (*token == '\'' || *token == '\"')
      token++;

    if (stripver)
      *(token + strcspn(token, "=<>\"\'")) = '\0';
    else {
      char *ptr = token + strlen(token) - 1;
      if (*ptr == '\'' || *ptr == '\"' )
        *ptr = '\0';
    }

    if (config->verbose >= 2)
      fprintf(stderr, "::DEBUG Adding Depend: %s\n", token);

    if (alpm_list_find_str(deplist, token) == NULL)
      deplist = alpm_list_add(deplist, strdup(token));

    token = strtok(NULL, " \n");
  }

  return deplist;
}

void pkgbuild_extinfo_get(char **pkgbuild, alpm_list_t **details[]) {
  char *lineptr, *arrayend;

  lineptr = *pkgbuild;

  do {
    strtrim(++lineptr);
    if (*lineptr == '#')
      continue;

    alpm_list_t **deplist;
    if (STR_STARTS_WITH(lineptr, PKGBUILD_DEPENDS))
      deplist = details[PKGDETAIL_DEPENDS];
    else if (STR_STARTS_WITH(lineptr, PKGBUILD_MAKEDEPENDS))
      deplist = details[PKGDETAIL_MAKEDEPENDS];
    else if (STR_STARTS_WITH(lineptr, PKGBUILD_OPTDEPENDS))
      deplist = details[PKGDETAIL_OPTDEPENDS];
    else if (STR_STARTS_WITH(lineptr, PKGBUILD_PROVIDES))
      deplist = details[PKGDETAIL_PROVIDES];
    else if (STR_STARTS_WITH(lineptr, PKGBUILD_REPLACES))
      deplist = details[PKGDETAIL_REPLACES];
    else if (STR_STARTS_WITH(lineptr, PKGBUILD_CONFLICTS))
      deplist = details[PKGDETAIL_CONFLICTS];
    else
      continue;

    arrayend = strstr(lineptr, ")\n");
    *arrayend  = '\0';

    if (deplist) {
      char *arrayptr = strchr(lineptr, '(') + 1;
      *deplist = parse_bash_array(*deplist, &arrayptr, FALSE);
    }

    lineptr = arrayend + 1;
  } while ((lineptr = strchr(lineptr, '\n')));
}

int get_pkg_dependencies(const char *pkg) {
  const char *dir;
  char *pkgbuild_path, *buffer;
  int ret = 0;

  if (config->download_dir == NULL)
    dir = getcwd(NULL, 0);
  else
    dir = realpath(config->download_dir, NULL);

  pkgbuild_path = calloc(1, PATH_MAX + 1);
  snprintf(pkgbuild_path, strlen(dir) + strlen(pkg) + 11, "%s/%s/PKGBUILD", dir, pkg);

  buffer = get_file_as_buffer(pkgbuild_path);
  if (! buffer) {
    if (config->color)
      cfprintf(stderr, "%<::%> Could not open PKGBUILD for dependency parsing.\n",
        config->colors->error);
    else
      fprintf(stderr, "!! Could not open PKGBUILD for dependency parsing.\n");
    return 1;
  }

  alpm_list_t *deplist = NULL;

  alpm_list_t **pkg_details[6];
  pkg_details[PKGDETAIL_DEPENDS] = &deplist;
  pkg_details[PKGDETAIL_MAKEDEPENDS] = &deplist;

  pkgbuild_extinfo_get(&buffer, pkg_details);

  free(buffer);

  if (!config->quiet && config->verbose >= 1) {
    if (config->color)
      cprintf("\n%<::%> Fetching uninstalled dependencies for %<%s%>...\n",
        config->colors->info, config->colors->pkg, pkg);
    else
      printf("\n:: Fetching uninstalled dependencies for %s...\n", pkg);
  }

  alpm_list_t *i = deplist;
  for (i = deplist; i; i = i->next) {
    const char* depend = i->data;
    if (config->verbose >= 2)
      printf("::DEBUG Attempting to find %s\n", depend);

    if (alpm_db_get_pkg(db_local, depend)) { /* installed */
      if (config->verbose >= 2)
        printf("::DEBUG %s is installed\n", depend);

      continue;
    }

    if (is_in_pacman(depend)) /* available in pacman */
      continue;

    /* can't find it, check the AUR */
    alpm_list_t *results = query_aur_rpc(AUR_QUERY_TYPE_INFO, depend);
    if (results) {

      if (config->verbose >= 2)
        printf("::DEBUG %s is in the AUR\n", depend);

      ret++;
      aur_get_tarball(results->data);

      aur_pkg_free(results->data);
      alpm_list_free(results);
    }
    /* Silently ignore packages that can't be found anywhere 
     * TODO: Maybe not silent if verbose? */
  }

  /* Cleanup */
  FREELIST(deplist);
  FREE(dir);
  FREE(pkgbuild_path);

  return ret;
}

/* vim: set ts=2 sw=2 et: */