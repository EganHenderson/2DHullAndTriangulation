/* This is an implementation of a 2D convex hull. With single and peel modes.
* A basic window is created, with some commands printed to the console window.
* From here, the user can create either a random set of 100 points, with incrementation of 100 possible as well, or use the mouse to create a set of points.
* After the points are set, the user can perform a single convex hull, or do a hull peel.
* The convex hull simply connects all the exterior points to encapsulate all interior points.
* The hull peel performs this until there are no points left. Removing the points used in the edges each time a convex hull is completed.
* After a hull peel is completed, the number of points and edges is printed to the console for reference.
*/

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <freeglut.h>
#include <FreeImage.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

//the point structure
typedef struct
{
	int x, y; //x and y coordinates within the window
} point;

//the edge structure
typedef struct
{
	point p1, p2; //the two points making up an edge
} edge;

//the global structure
typedef struct
{
	int w, h; //w for width and h for height of the window
	int n; //number of points to create
	vector<point> points; //vector of points
	vector<point> coords; //vector for all possible coordinates
	vector<edge> edges; //vector of edges
	bool mouseDraw; //true when drawing points with the mouse
	bool shuffled; //true when the coords vector has been shuffled
	int clusters; //the number of clusters to create
	bool clustering; //true when we are clustering points
} glob;
glob global;

//enums for the menu buttons/options
enum {
	MENU_QUIT, MENU_RANDOM, MENU_CONVEX, MENU_PEEL, MENU_INCREMENT, MENU_MOUSE, MENU_CLUSTER, MENU_CLUSTER_INCREMENT
};

//this method creates a set of random points within the bounds of the window
//the point and edge vectors are cleared to ensure the new points are added to an empty vector
void random()
{
	vector<point>().swap(global.points); //clear the points vector, getting rid of its contents and freeing some memory
	vector<edge>().swap(global.edges); //clear the edges vector, getting rid of its contents and freeing some memory
	random_shuffle(global.coords.begin(), global.coords.end()); //shuffle the coordinate vector

	//we have shuffled, so set the shuffle bool to true if it isn't already
	if (!global.shuffled)
		global.shuffled = true;

	//create 'n' random points from the vector of possible points based on the window size
	for (int i = 0; i < global.n; i++)
		global.points.push_back(point{ global.coords[i].x, global.coords[i].y }); //add the new point to the point vector

	glutPostRedisplay(); //redisplay the window
}

//this method creates a lattice of points. not scaled or based on window size, just here for testing :)
//only used for development, not necessary for the program/assignment
void lattice()
{
	vector<point>().swap(global.points); //clear the points vector, getting rid of its contents and freeing some memory
	vector<edge>().swap(global.edges); //clear the edges vector, getting rid of its contents and freeing some memory

	//create 100 points in a 10x10 lattice
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 10; j++)
			global.points.push_back(point{ 100 + i * 10, 100 + j * 10 }); //add the new point to the point vector

	glutPostRedisplay(); //redisplay the window
}

//create a point where the mouse clicks
//this method ensures that each point is unique and places the points correctly based on the mouse position in the window
//the y value has to be essentially inversed as 0 in the window is bottom left and 0 for the mouse position is top left
void draw_mouse_point(int x, int y)
{
	y = global.h - 10 - y; //subtract the value of y from the max y coordinate

	//ensure the point is unique from all other points
	for (const point& p : global.points)
		if (p.x == x && p.y == y)
			return;

	global.points.push_back(point{ x, y }); //add the point to the global points vector
	vector<edge>().swap(global.edges); //clear the edges vector, getting rid of its contents and freeing some memory

	glutPostRedisplay(); //redisplay the window
}

//this method is called whenever opengl calls the mouse fun
//it calls the draw_mouse_point when the user is creating points with the mouse and has left clicked somewhere in the window
void mouse(int bin, int state, int x, int y)
{
	if (global.mouseDraw && bin == GLUT_LEFT_BUTTON && state == GLUT_DOWN) draw_mouse_point(x, y);
}

//this method is an implementation of the QuickHull algorithm
//the coefficients to calculated the distance are calculated first
//points for the max and direction vectors are created, and the ints for distance are initialized
//then all points are iterated through, finding the point with the max distance from the line
//if no point is found, that means either all points are interior to the line or there are colinear points
//that means that the edge is added to the global edge vector
//if a point is found, quick_hull is called on the two lines from pMax to the original two points
void quick_hull(vector<point> points, point p1, point p2)
{
	//set the coefficients
	int a = p1.y - p2.y;
	int b = p2.x - p1.x;
	int c = p1.x * p2.y - p1.y * p2.x;

	//initalize the points and ints
	point pMax = point{ 0, 0 };
	int maxD = 0;
	int d;
	point dir_p1_p, dir_p_p2;

	//iterate through all points, finding the max distance points
	for (const point& p : points)
	{
		if (!(p.x == p1.x && p.y == p1.y || p.x == p2.x && p.y == p2.y))
		{
			d = a * p.x + b * p.y + c;

			if (d > maxD)
			{
				pMax = p;
				maxD = d;
			}
		}
	}

	//if no point is found, add the edge to the vector
	if (maxD == 0)
	{
		global.edges.push_back(edge{ p1, p2 });
		return;
	}

	//recursively call quick_hull
	quick_hull(points, p1, pMax);
	quick_hull(points, pMax, p2);
}

//this method creates a convex hull using the quick hull algorithm
//the method finds the point with the minimum x and maximum x values
//then, quick_hull is called for both directions of the line, ensuring we create a top and bottom to the hull
void convex_hull(vector<point> points)
{
	//if there are less than three points, we cannot create a convex hull so immediately stop
	if (points.size() < 3)
		return;

	//initialize the min and max points
	point minPoint;
	minPoint.x = global.w;
	point maxPoint;
	maxPoint.x = -1;

	//iterate through all points and find the min and max
	for (const point& p : points)
	{
		if (p.x < minPoint.x)
			minPoint = p;
		if (p.x > maxPoint.x)
			maxPoint = p;
	}

	//call quick hull for both directions of the line
	quick_hull(points, minPoint, maxPoint);
	quick_hull(points, maxPoint, minPoint);

	glutPostRedisplay(); //redisplay the window
}

//this method conducts a hull peel
//while there are at least three points, call convex hull, then remove the points from the hull edges and call convex hull again
//this checks if any point in the points vector is in the edges vector by finding the distance of a point from the current edge
//after finding which points are not already in a convex hull, the points vector is set to a new set of points
void peel(vector<point> points)
{
	//initialize the variables
	vector<point> newPoints;
	bool pointExists;
	int a, b, c, d;

	//as long as there are at least three points, we can do a convex hull
	while (points.size() > 2)
	{
		convex_hull(points); //call convex hull to do a hull of the current vector of points
		vector<point>().swap(newPoints); //clear the new points vector, getting rid of its contents and freeing some memory

		//iterate through all points to determine which points already are in a convex hull
		for (const point& p : points)
		{
			pointExists = false; //make sure we don't think the point already exists before checking

			//iterate though all edges to determine if the point p is already on a calculated edge
			for (const edge& e : global.edges)
			{
				//calculate the distance of the point from the edge
				a = e.p1.y - e.p2.y;
				b = e.p2.x - e.p1.x;
				c = e.p1.x * e.p2.y - e.p1.y * e.p2.x;
				d = a * p.x + b * p.y + c;

				//if the point lies on the line, it already is in the edge vector, and we can say it exists, so stop looking for a matching edge point
				if (d == 0)
				{
					pointExists = true;
					break;
				}
			}

			//if we don't find the point on any edge, add it to the new points vector
			if (!pointExists)
				newPoints.push_back(p);
		}		

		vector<point>().swap(points); //clear the points vector, getting rid of its contents and freeing some memory
		//add all points from the new points vector to the global points vector
		for (int i = 0; i < newPoints.size(); i++)
			points.push_back(newPoints[i]);
	}
	
	//let the user know how many points were used and how many edges were created
	cout << "Peel completed with " << global.points.size() << " points and " << global.edges.size() << " edges." << endl;

	//clear the global points vector as it looks nicer without the points when a peel is performed
	//only do this if we are not clustering points
	if (!global.clustering)
	{
		vector<point>().swap(global.points);
		glutPostRedisplay();
	}
}

//this method returns the distance between two points using pythagorean theorem
int pythagorean(point p1, point p2)
{
	return sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
}

//this method creates cluster peels based on the set number of clusters to create
//it finds the first point in the global points vector, finds the nearest points, and performs a hull peel on that
//then it removes those points and continues until all points have been peeled
void cluster_peel()
{
	//initialize variables
	vector<point> clusterPoints, newPoints;
	int dist;
	bool pointExists;
	global.clustering = true;

	//create the number of clusters set by global.clusters (default 5)
	for (int i = 0; i < global.clusters; i++)
	{
		dist = 1; //reset the distance
		vector<point>().swap(clusterPoints); //clear the clusterPoints vector and free some memory
		vector<point>().swap(newPoints);
		clusterPoints.push_back(global.points[0]);

		//add the closest n / clusters points to the clusterPoints vector
		//very inefficiently checks all progressive distances until enough points are added
		while (clusterPoints.size() < global.n / global.clusters)
		{
			for (const point& p : global.points)
			{
				if (clusterPoints.size() >= global.n / global.clusters)
					break;

				if (pythagorean(clusterPoints[0], p) == dist)
					clusterPoints.push_back(p);
			}

			dist++;
		}

		peel(clusterPoints); //peel the points

		//ensure the global points vector no longer contains points that were added to the cluster
		//iterate through all points to determine which points already are in the cluster
		for (const point& p1 : global.points)
		{
			pointExists = false; //make sure we don't think the point already exists before checking

			//iterate though all points to determine if the point p1 is already in the cluster
			for (const point& p2 : clusterPoints)
				if (p1.x == p2.x && p1.y == p2.y)
					pointExists = true;

			//if we don't find the point, add it to the new points vector
			if (!pointExists)
				newPoints.push_back(p1);
		}

		vector<point>().swap(global.points); //clear the points vector, getting rid of its contents and freeing some memory
		//add all points from the new points vector to the global points vector
		for (int j = 0; j < newPoints.size(); j++)
			global.points.push_back(newPoints[j]);
	}

	global.clustering = false;
}

//this method draws the points and edges to the window, showing the work done by the hull algorithms
void draw()
{
	glClear(GL_COLOR_BUFFER_BIT); //make sure to clear the screen before drawing new points

	glPointSize(3.0); //set the size of the points larger so they are easier to see
	glColor3f(1.0, 1.0, 1.0); //set them to white

	//begin drawing points on screen
	glBegin(GL_POINTS);

	//draw all the points in the global points vector onto the screen
	for (const point& p : global.points)
		glVertex2i(p.x, p.y);

	//stop drawing points
	glEnd();

	//begin drawing lines on screen
	glBegin(GL_LINES);

	//draw all the edges in the global edges vector onto the screen
	for (const edge& e : global.edges)
	{
		glVertex2i(e.p1.x, e.p1.y);
		glVertex2i(e.p2.x, e.p2.y);
	}

	//stop drawing and flush the buffer to screen
	glEnd();
	glFlush();
}

//this method increments the global n value by 100
//this is used to add 100 more points to the global points vector for generating random points
//it ensures that the number of points will not exceed the size of the coords vector of all possible points
void increment_n()
{
	//if the global n is already at the size of the coordinate vector, inform the user and don't increment
	if (global.n == global.coords.size())
	{
		cout << "No more than " << global.n << " points allowed at current screen size!" << endl;
		return;
	}

	//check if the program has already shuffled the coords vector, if not, shuffle it
	if (!global.shuffled)
	{
		random_shuffle(global.coords.begin(), global.coords.end()); //shuffle the coordinate vector
		global.shuffled = true;
		global.n -= 100;
	}

	//add 100 points to the global points vector
	global.n += 100;
	for (int i = global.points.size(); i < global.n; i++)
		global.points.push_back(point{ global.coords[i].x, global.coords[i].y }); //add the new point to the point vector
		
	vector<edge>().swap(global.edges); //clear the edges vector, getting rid of its contents and freeing some memory
	glutPostRedisplay(); //redisplay the window
}

//this method increments the global cluster value by 5
//this is used to add 5 more points to the global clusters for generating clusters
//it ensures that the number of points will not exceed the size of the points vector
void increment_clusters()
{
	//if the global n is already at the size of the coordinate vector, inform the user and don't increment
	if (global.n == global.points.size() / 2)
	{
		cout << "No more than " << global.n / 2<< " clusters allowed at current screen size!" << endl;
		return;
	}

	//add 5 points to the global clusters
	global.clusters += 5;
}

//this method sets the global mouse draw bool to its opposite, and clears the global points vector is mouseDraw was set to true
void set_mouse_draw()
{
	global.mouseDraw = !global.mouseDraw;
	if (global.mouseDraw)
		vector<point>().swap(global.points);
}

/*glut keyboard function*/
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 0x1B:
	case'q':
	case 'Q':
		exit(0);
		break;
	case 'r':
	case 'R':
		random();
		break;
	case 'm':
	case 'M':
		set_mouse_draw();
		break;
	case 'c':
	case 'C':
		convex_hull(global.points);
		break;
	case 'p':
	case 'P':
		peel(global.points);
		break;
	case 'l':
	case 'L':
		lattice();
		break;
	case 'a':
	case 'A':
		increment_n();
		break;
	case 'u':
	case 'U':
		cluster_peel();
		break;
	case 'y':
	case 'Y':
		increment_clusters();
		break;
	}
}//keyboard

//Glut menu callback function
void menuFunc(int value)
{
	switch (value)
	{
	case MENU_QUIT:
		exit(0);
		break;
		break;
	case MENU_RANDOM:
		random();
		break;
	case MENU_CONVEX:
		convex_hull(global.points);
		break;
	case MENU_PEEL:
		peel(global.points);
		break;
	case MENU_INCREMENT:
		increment_n();
		break;
	case MENU_MOUSE:
		set_mouse_draw();
		break;
	case MENU_CLUSTER:
		cluster_peel();
		break;
	case MENU_CLUSTER_INCREMENT:
		increment_clusters();
		break;
	}

	glutPostRedisplay();
}//menuFunc

//show the keys for actions in the terminal
void show_keys()
{
	printf("Q:quit\nR:random\nM:mouse selection\nA:Add 100 points\nC:convex hull\nP:peel\nU:cluster peel\nY:increment clusters\n");
}

//Glut menu set up
void init_menu()
{
	//menu for overall menu options
	int main_menu = glutCreateMenu(&menuFunc);
	glutAddMenuEntry("Random Points", MENU_RANDOM);
	glutAddMenuEntry("Add 100 Points", MENU_INCREMENT);
	glutAddMenuEntry("Mouse Points", MENU_MOUSE);
	glutAddMenuEntry("Convex Hull", MENU_CONVEX);
	glutAddMenuEntry("Hull Peel", MENU_PEEL);
	glutAddMenuEntry("Cluster Peel", MENU_CLUSTER);
	glutAddMenuEntry("Increment Clusters", MENU_CLUSTER_INCREMENT);
	glutAddMenuEntry("Quit", MENU_QUIT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

//initialize the vector for coordinates
void initializeVector()
{
	//initialize coordinate vector, going through all possible combinations of x and y (making sure points are only drawn within the visual bounds of the screen!)
	for (int x = 1; x < global.w - 9; x++)
		for (int y = 1; y < global.h - 9; y++)
			global.coords.push_back(point{ x, y }); //add the point to the global vector of points
}

//what runs the whole show
int main(int argc, char** argv)
{
	srand(static_cast <unsigned> (time(0))); //set a new random seed

	global.w = 1000; //width and height set to 1000 x 800
	global.h = 800;
	global.n = 100; //set default number of points to 100 (maximum based on window size - 774,200)
	global.clusters = 5; //set default number of clusters to create
	initializeVector(); //initialize the coordinate vectors

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);

	glutInitWindowSize(global.w, global.h);
	glutCreateWindow("2D Hull Peeler");
	glShadeModel(GL_SMOOTH);
	glutKeyboardFunc(keyboard);
	glMatrixMode(GL_PROJECTION);
	gluOrtho2D(0, global.w, 0, global.h);

	init_menu();
	show_keys();

	glutMouseFunc(mouse);
	glutDisplayFunc(draw);

	glutMainLoop();
}