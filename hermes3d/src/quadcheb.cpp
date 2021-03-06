// This file is part of Hermes3D
//
// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Email: hpfem-group@unr.edu, home page: http://hpfem.org/.
//
// Hermes3D is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// Hermes3D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes3D; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "h3d_common.h"
#include "quadcheb.h"
#include "refdomain.h"

#include "../../hermes_common/error.h"
#include "../../hermes_common/trace.h"
#include "../../hermes_common/callstack.h"


//// QuadChebTetra  ///////////////////////////////////////////////////////////////////////////////

QuadChebTetra::QuadChebTetra() {
	_F_
#ifdef WITH_TETRA
	mode = HERMES_MODE_TET;

	max_edge_order = 10;
	max_face_order = Ord2(10);
	max_order = Ord3(10);

	QuadPt3D *pt;
	for (int o = 0; o < 10; o++) {
		Ord3 ord(o);
		int idx = ord.get_idx();
		(*np)[idx] = (o + 1) * (o + 2) * (o + 3) / 6;
    if((*tables)[idx] != NULL)
      delete [] (*tables)[idx];
		(*tables)[idx] = pt = new QuadPt3D[(*np)[idx]];

		for (int i = o, m = 0; i >= 0; i--)				// z
			for (int j = o; j >= o - i; j--)			// y
				for (int k = o; k >= o - j + o - i; k--, m++) {	// x
					assert(m < (*np)[idx]);
					pt[m].x = o ? cos(k * M_PI / o) : 1.0;
					pt[m].y = o ? cos(j * M_PI / o) : 1.0;
					pt[m].z = o ? cos(i * M_PI / o) : 1.0;
					pt[m].w = 1.0;
				}
	}
#endif
}

QuadChebTetra::~QuadChebTetra() {
	_F_
  for(std::map<unsigned int, QuadPt3D *>::iterator it = tables->begin(); it != tables->end(); it++)
    delete [] it->second;
  for(std::map<unsigned int, std::map<unsigned int, QuadPt3D *>*>::iterator it = edge_tables->begin(); it != edge_tables->end(); it++) {
    for(std::map<unsigned int, QuadPt3D *>::iterator it_inner = it->second->begin(); it_inner != it->second->end(); it_inner++)
      delete [] it_inner->second;
    delete it->second;
  }
  for(std::map<unsigned int, std::map<unsigned int, QuadPt3D *>*>::iterator it = face_tables->begin(); it != face_tables->end(); it++) {
    for(std::map<unsigned int, QuadPt3D *>::iterator it_inner = it->second->begin(); it_inner != it->second->end(); it_inner++)
      delete [] it_inner->second;
    delete it->second;
  }
  if(vertex_table != NULL)
    delete [] vertex_table;
  np->clear();
  delete np;
  np_edge->clear();
  delete np_edge;
  np_face->clear();
  delete np_face;
}

//// QuadChebHex  /////////////////////////////////////////////////////////////////////////////////

QuadChebHex::QuadChebHex() {
	_F_
#ifdef WITH_HEX
	mode = HERMES_MODE_HEX;

	max_edge_order = 10;
	max_face_order = Ord2(10, 10);
	max_order = Ord3(10, 10, 10);

	int i, j, k;
	for (i = 0; i <= 10; i++)
		for (j = 0; j <= 10; j++)
			for (k = 0; k <= 10; k++) {
				Ord3 o(i, j, k);
				(*np)[o.get_idx()] = (i + 1) * (j + 1) * (k + 1);
			}
#endif
}

QuadChebHex::~QuadChebHex() {
	_F_
  for(std::map<unsigned int, QuadPt3D *>::iterator it = tables->begin(); it != tables->end(); it++)
    delete [] it->second;
  for(std::map<unsigned int, std::map<unsigned int, QuadPt3D *>*>::iterator it = edge_tables->begin(); it != edge_tables->end(); it++) {
    for(std::map<unsigned int, QuadPt3D *>::iterator it_inner = it->second->begin(); it_inner != it->second->end(); it_inner++)
      delete [] it_inner->second;
    delete it->second;
  }
  for(std::map<unsigned int, std::map<unsigned int, QuadPt3D *>*>::iterator it = face_tables->begin(); it != face_tables->end(); it++) {
    for(std::map<unsigned int, QuadPt3D *>::iterator it_inner = it->second->begin(); it_inner != it->second->end(); it_inner++)
      delete [] it_inner->second;
    delete it->second;
  }
  if(vertex_table != NULL)
    delete [] vertex_table;
  np->clear();
  delete np;
  np_edge->clear();
  delete np_edge;
  np_face->clear();
  delete np_face;
}

void QuadChebHex::calc_table(const Ord3 &order) {
	_F_
#ifdef WITH_HEX
	QuadPt3D *pt;

	int idx = order.get_idx();
  if((*tables)[idx] != NULL)
    delete [] (*tables)[idx];
	(*tables)[idx] = pt = new QuadPt3D[(*np)[idx]];

	for (int i = order.z, m = 0; i >= 0; i--)
		for (int j = order.y; j >= 0; j--)
			for (int k = order.x; k >= 0; k--, m++) {
				pt[m].x = order.x ? cos(k * M_PI / order.x) : 1.0;
				pt[m].y = order.y ? cos(j * M_PI / order.y) : 1.0;
				pt[m].z = order.z ? cos(i * M_PI / order.z) : 1.0;
				pt[m].w = 1.0;
			}
#endif
}
