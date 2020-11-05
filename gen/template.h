/* Copyright 2019-2020 National Research Foundation (SARAO)
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * This file is automatically generated. Do not edit.
 */

#ifndef SPEAD2_COMMON_LOADER_{{ name | upper }}_H
#define SPEAD2_COMMON_LOADER_{{ name | upper }}_H

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <spead2/common_features.h>

#if {{ guard }}

{% for header in headers -%}
#include <{{ header }}>
{% endfor %}

{% for node in nodes -%}
#undef {{ node.name }}
{% endfor %}

namespace spead2
{

{% for node in nodes -%}
extern {{ node | ptr | gen }};
{% if node.name in optional -%}
bool has_{{ node.name }}();
{% endif %}
{% endfor %}

} // namespace spead2

#endif // {{ guard }}
#endif // SPEAD2_COMMON_LOADER_{{ name | upper }}_H
