/*
  Copyright 1993-2014 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/adjacency_list.hpp>
#include "edmonds_optimum_branching.hpp"

// Define a directed graph type that associates a weight with each
// edge. We store the weights using internal properties as described
// in BGL.
typedef boost::property<boost::edge_weight_t, double>       EdgeProperty;
typedef boost::adjacency_list<boost::listS,
                              boost::vecS,
                              boost::directedS,
                              boost::no_property,
                              EdgeProperty>                 Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor       Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor         Edge;

static double **cost_array;
static int N;

int main(int argc, char **argv)
{
	FILE *fp;
	int k, u, v;
	char **node_name, name_buf[2048], *root_name=NULL;
	Vertex *root=static_cast<Vertex *>(0);

	if (strncmp(argv[argc-1], "-root=", 6) == 0)
	{
		root_name = argv[argc-1]+6;
		argc--;
	}
	if (argc == 1)
		fp = stdin;
	else
	{
		fp = fopen(argv[1], "rb");
		if (fp == NULL)
		{
			fprintf(stderr, "Cannot open %s.\n", argv[1]);
			exit(1);
		}
	}
	if (fscanf(fp, "%d nodes\n", &N)!=1 ||
			fgets(name_buf, sizeof(name_buf), fp)==NULL)
	{
		fprintf(stderr, "error reading input file\n");
		exit(1);
	}
	node_name = (char **)malloc(N*sizeof(char *));
	cost_array = (double **)malloc(N*sizeof(double *));
	for (u=k=0; u<N; u++)
	{
		node_name[u] = name_buf+k;
		if (k == sizeof(name_buf))
		{
			fprintf(stderr, "names missing in input file\n");
			exit(1);
		}
		while (!isspace(name_buf[k]))
			k++;
		name_buf[k++] = 0;
		cost_array[u] = (double *)malloc(N*sizeof(double));
		for (v=0; v<N; v++)
		{
			if (fscanf(fp, " %lf", cost_array[u]+v) != 1)
			{
				fprintf(stderr, "error reading input file\n");
				exit(1);
			}
		}
		fscanf(fp, "\n");
	}

    // Graph with N vertices    
    Graph G(N);

    // Create a vector to keep track of all the vertices and enable us
    // to index them. As a side note, observe that this is not
    // necessary since Vertex is probably an integral type.
    std::vector<Vertex> the_vertices;
    BOOST_FOREACH (Vertex v, vertices(G))
    {
        the_vertices.push_back(v);
    }
    
    // add edges with weights to the graph
	for (u=0; u<N; u++)
	{
		if (root_name && strcmp(node_name[u], root_name)==0)
			root = &the_vertices[u];
		for (v=0; v<N; v++)
			if (v != u)
				add_edge(the_vertices[u],the_vertices[v], cost_array[u][v], G);
	}

	if ((root==NULL) != (root_name==NULL))
	{
		fprintf(stderr, "No such root.\n");
		exit(-1);
	}

    // This is how we can get a property map that gives the weights of
    // the edges.
    boost::property_map<Graph, boost::edge_weight_t>::type weights =
        get(boost::edge_weight_t(), G);
    
    // This is how we can get a property map mapping the vertices to
    // integer indices.
    boost::property_map<Graph, boost::vertex_index_t>::type vertex_indices =
        get(boost::vertex_index_t(), G);


    // Find the minimum branching.
    std::vector<Edge> branching;
    edmonds_optimum_branching<false, true, true>(
		G,
		vertex_indices,
		weights,
		root,
		root? root+1: root,
		std::back_inserter(branching));
    
    // Print the edges of the minimum branching
    std::cout << "This is the minimum branching\n";
	double tree_cost=0;
    BOOST_FOREACH (Edge e, branching)
    {
		tree_cost += cost_array[boost::source(e, G)][boost::target(e, G)];
        std::cout << "(" << node_name[boost::source(e, G)] << ", "
                  << node_name[boost::target(e, G)] << ")\n";
    }
	std::cout << "Tree cost = " << tree_cost << "\n";

	exit(0);
}
