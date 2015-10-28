#include "AStar.h"

namespace Pathfinding {

    // TODO: check for arrival at goal?
    void a_star(const Vector3* goal, std::vector<AStarNode*> open_list, std::vector<AStarNode*> closed_list) {

        while (open_list.size() > 0) {
            AStarNode* smallest_f_node;
            float smallest_f = INT_MAX;
            for each (AStarNode* n in open_list)
            {
                float nf = distance_squared(n->polygon->getCentre(), goal);
                if (nf < smallest_f) {
                    smallest_f_node = n;
                    smallest_f = nf;
                }
            }

            for (int i = 0; i < open_list.size(); i++) {
                if (*(open_list[i]) == *(smallest_f_node)) {
                    open_list.erase(open_list.begin() + i);
                    break;
                }
            }

            for each (ConvexPolygon* neighbour in smallest_f_node->polygon->neighbours) {
                AStarNode* neighbour_node = new AStarNode(neighbour);
                neighbour_node->parent = smallest_f_node;

                const Vector3* smallest_f_node_pt = smallest_f_node->polygon->getCentre();

                neighbour_node->g = 
                    smallest_f_node->g + 
                    distance_squared(neighbour_node->polygon->getCentre(), smallest_f_node_pt);

                neighbour_node->h = distance_squared(neighbour_node->polygon->getCentre(), goal);
                neighbour_node->f = neighbour_node->g + neighbour_node->h;

                for each (AStarNode* n in open_list)  {
                    if (neighbour_node->polygon == n->polygon) {
                        if (n->f < neighbour_node->f) {
                            delete neighbour_node;
                            continue;
                        }
                    }
                }
                for each (AStarNode* n in closed_list)  {
                    if (neighbour_node->polygon == n->polygon) {
                        if (n->f < neighbour_node->f) {
                            delete neighbour_node;
                            continue;
                        }
                    }
                }
                open_list.push_back(neighbour_node);
            }
            closed_list.push_back(smallest_f_node);
        }
    }
};