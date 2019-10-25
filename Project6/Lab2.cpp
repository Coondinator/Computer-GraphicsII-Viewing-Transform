#include<fstream>
#include <iostream>
#include<sstream>
#include <glut.h>
#include<vector>
#include<algorithm>
#include<string>
#include<math.h>
#define Pi 3.1415926
using namespace std;

int OutputPolygon[2750][4]={ 0 };// The global array to store the polygons after back-face culling
float final[2000][3] = { 0 }; // The global array to store the vertces data with z after all viewing transformation
float intfinal[2000][3] = { 0 };

struct Line
{
	 int highpoint[3];
	 int lowpoint[3];
	 float Reciprocal_K;
	 float DeltaZ;
};

struct EdgeList
{
	float Reciprocal_K;
	float DeltaZ;
	int Ymax;
	int X;
	int Zmax;
	EdgeList *next;
};

struct ActiveEdgeTable 
{
	int y;
	EdgeList *head;
};

typedef struct Bucket 
{
	int y;
	Bucket *next=NULL;
	EdgeList *head=NULL;
}SortedEdgeTable;

struct Pixel
{
	int x;
	int y;
	float z;
	float R;
	float G;
	float B;
};Pixel **ZB;


Line* CreateLines(float K[2],float D[2],float F[2]) {
	Line *lines = (Line*)malloc( 3 * sizeof(Line));
	lines[0].Reciprocal_K = (K[1] - D[1]) / (K[0] - D[0]);
	lines[0].DeltaZ = (K[2] - D[2]) / (K[1] - D[1]);
	lines[1].Reciprocal_K = (D[1] - F[1]) / (D[0] - F[0]);
	lines[1].DeltaZ = (D[2] - F[2]) / (D[1] - F[1]);
	lines[2].Reciprocal_K = (K[1] - F[1]) / (K[0] - F[0]);
	lines[3].DeltaZ = (K[2] - F[2]) / (K[1] - F[1]);

	if (K[1] > D[1])
	{
		lines[0].highpoint[0] = K[0];
		lines[0].highpoint[1] = K[1];
		lines[0].highpoint[2] = K[2];
		lines[0].lowpoint[0] = D[0];
		lines[0].lowpoint[1] = D[1];
		lines[0].lowpoint[2] = D[2];
	}
	else
	{
		lines[0].highpoint[0] = K[0];
		lines[0].highpoint[1] = K[1];
		lines[0].highpoint[2] = K[2];
		lines[0].lowpoint[0] = D[0];
		lines[0].lowpoint[1] = D[1];
		lines[0].lowpoint[2] = D[2];
	}
	if (D[1] > F[1])
	{
		lines[1].highpoint[0] = D[0];
		lines[1].highpoint[1] = D[1];
		lines[1].highpoint[2] = D[2];
		lines[1].lowpoint[0] = F[0];
		lines[1].lowpoint[1] = F[1];
		lines[1].lowpoint[2] = F[2];
	}
	else
	{
		lines[1].highpoint[0] = F[0];
		lines[1].highpoint[1] = F[1];
		lines[1].highpoint[2] = F[1];
		lines[1].lowpoint[0] = D[0];
		lines[1].lowpoint[1] = D[1];
		lines[1].lowpoint[2] = D[2];
	}
	if (K[1] > F[1])
	{
		lines[1].highpoint[0] = K[0];
		lines[1].highpoint[1] = K[1];
		lines[1].highpoint[2] = K[2];
		lines[1].lowpoint[0] = F[0];
		lines[1].lowpoint[1] = F[1];
		lines[1].lowpoint[2] = F[2];
	}
	else
	{
		lines[1].highpoint[0] = F[0];
		lines[1].highpoint[1] = F[1];
		lines[1].highpoint[2] = F[2];
		lines[1].lowpoint[0] = K[0];
		lines[1].lowpoint[1] = K[1];
		lines[1].lowpoint[2] = K[2];
	}

	return lines;
		/*edges[0].Ymax = (K[1]-D[1])>0? K[1]:D[1];
		edges[0].Xmin = (K[0] - D[0]) > 0 ? D[0] : K[0];
		edges[0].Reciprocal_K = (K[0] - D[0]) / (K[1] - D[1]);

		edges[1].Ymax = (D[1] - F[1]) > 0 ? D[1] : F[1];
		edges[1].Xmin = (D[0] - F[0]) > 0 ? F[0] : D[0];
		edges[1].Reciprocal_K = (D[0] - F[0]) / (D[1] - F[1]);

		edges[2].Ymax = (K[1] - F[1]) > 0 ? K[1] : F[1];
		edges[2].Xmin = (K[0] - F[0]) > 0 ? F[0] : K[0];
		edges[2].Reciprocal_K = (K[0] - F[0]) / (K[1] - F[1]);*/
}

int* get_lowest_point(Line lines[],int n)
{
	int lowest_point[3] = { lines[0].lowpoint[0],lines[0].lowpoint[1],lines[0].lowpoint[2]};
	for (int i = 1; i < n;++i) {
		int low_point[2] = { lines[i].lowpoint[0],lines[i].lowpoint[1] };
		if (low_point[1]<lowest_point[1])
		{
			lowest_point[0]= low_point[0];
			lowest_point[1] = low_point[1];
			lowest_point[2] = low_point[2];
		}
	}
	return lowest_point;
}

int* get_highest_point(Line lines[], int n)
{
	int highest_point[2] = { lines[0].highpoint[0],lines[0].highpoint[1] };
	for (int i = 1; i < n; ++i) {
		int high_point[2] = { lines[i].lowpoint[0],lines[i].lowpoint[1] };
		if (high_point[1] > highest_point[1])
		{
			highest_point[0] = high_point[0];
			highest_point[1] = high_point[1];
		}
	}
	return highest_point;
}

void sort(Line lines[],int n) 
{
	for (int i=0;i<n;++i) 
	{
		int min = i;
		for (int j = i + 1; j < n; ++j) {
			if (lines[j].lowpoint[1] < lines[min].lowpoint[1])
				min = j;
		}
		Line t = lines[i]; lines[i] = lines[min]; lines[min] = t;
	}

	for (int i = 0; i < n; ++i) {
		int min = i;
		for (int j = i + 1; j < n; ++j) {
			if (lines[j].lowpoint[0] < lines[min].lowpoint[0])
				min = j;
		}
		Line t = lines[i]; lines[i] = lines[min]; lines[min] = t;
	}

};

SortedEdgeTable* CreatEdgeList(Line lines[],int n)
{
	SortedEdgeTable* sorted_edge_table = (SortedEdgeTable*)malloc(sizeof SortedEdgeTable);
	sorted_edge_table->head= NULL;
	sorted_edge_table->next = NULL;
	sort(lines, n);
	int lowestpoint[2]; int *LPA = &lowestpoint[0]; LPA= get_lowest_point(lines, n);
	int highestpoint[2]; int *HPA = &highestpoint[0]; HPA = get_highest_point(lines, n);
	SortedEdgeTable* S = sorted_edge_table;
	for (int i=lowestpoint[1];i<highestpoint[1];++i) 
	{
		SortedEdgeTable* bucket = (Bucket*)malloc(sizeof Bucket);
		bucket->y = i;
		bucket->next = NULL;
		bucket->head = (EdgeList*)malloc(sizeof(EdgeList));
		bucket->head->next = NULL;
		EdgeList* p = bucket->head;
		for (int j = 0; j < n; ++j) 
		{
			if (lines[j].lowpoint[1] == i) 
			{
				EdgeList *q;
				q = new EdgeList;//EdgeList* q = (EdgeList*)malloc(sizeof EdgeList);
				q->Ymax = lines[j].highpoint[1];
				q->X = lines[j].lowpoint[0];
				q->Zmax = lines[j].highpoint[2];
				q->Reciprocal_K = lines[j].Reciprocal_K;
				q->DeltaZ = lines[j].DeltaZ;
				q->next = NULL;
				p->next = q;
				p = q;
			}
		}
		sorted_edge_table->next = bucket;
		sorted_edge_table = bucket;
	}
	return sorted_edge_table;
}

ActiveEdgeTable* Created_AET(SortedEdgeTable* edgetable) 
{
	ActiveEdgeTable* AET=(ActiveEdgeTable*)malloc(sizeof(ActiveEdgeTable));
	AET->y = edgetable->next->y;
	AET->head = new EdgeList;
	AET->head->next = NULL;

	EdgeList *EL = edgetable->head;
	EdgeList *AEL = AET->head;

	if (EL->next!=NULL)
	{
		EdgeList *P = new EdgeList;
		P->X = EL->next->X;
		P->Ymax = EL->next->Ymax;
		P->Reciprocal_K = EL->next->Reciprocal_K;
		P->next = NULL;
		AEL->next = P;
		AEL = P;
		EL = EL->next;
	}
	return AET;
}

void Add_EdgeList(ActiveEdgeTable* AET, EdgeList EL) 
{
	EdgeList* P = new EdgeList;
	P->X = EL.X;
	P->Ymax = EL.Ymax;
	P->Reciprocal_K = EL.Reciprocal_K;
	P->next = NULL;

	EdgeList *Q1 = AET->head;
	while (Q1->next!=NULL)
	{
	  EdgeList* Q2 = Q1->next;
	  if (Q2->X > EL.X || (EL.X == Q2->X&&EL.Reciprocal_K < Q2->Reciprocal_K))
	  {
		  Q1->next = Q2;
		  Q2->next = Q1;
		  return;
	  }
	}
}

ActiveEdgeTable* Update_AET(ActiveEdgeTable* AET,SortedEdgeTable* SET) 
{
	(AET->y)++;

	EdgeList* p = AET->head->next;
	while (p!=NULL)
	{
		p->X += p->Reciprocal_K;
		p = p->next;
	}
	SortedEdgeTable *q = SET;
	if (q != NULL)
	{
		EdgeList* T = q->head;
		while ((T=T->next)!=NULL)
		{
			Add_EdgeList(AET, *T);
		}
	}
	return AET;
}


void readTxt(string file,float C[][3],int P[][4]) //read the d-file
{
	ifstream infile;
	infile.open(file.data());   
	if (!infile) cout << "error" << endl;
	
	//vector <double> SJ;
	string str;
	int t = 1;
	int i = 0;
	int j = 0;
	while (getline(infile, str))
	{
		int num2, num3;
		istringstream iss(str);
		//std::cout << str << '\n';
		//SJ.push_back(stringToNum<float>(str));
	    if (t == 1) {
			string num1;
			iss >> num1;
			iss >> num2; //vertices numbers
			iss >> num3; //original polygons number
			t++;
			//std::cout << num2 << '\n';
		}
	    
		else if (t>=2&&t<=num2+1)//read the original vertices data
		{
			
			iss>>C[i][0];
			iss>>C[i][1];
			iss>>C[i][2];
			//std::cout << C[i][0] << '\t'<<"i="<<i<<'\n';
			i++;
			t++;
			//SJ.clear();
		}
		else if(t>=num2+2&&t<=num2+num3+1)// read the original polygons data
		{
			
			iss>>P[j][0];  
			iss>>P[j][1];
			iss>>P[j][2];
			iss>>P[j][3];
			j++;
			t++;
			//SJ.clear();
		}
	}
	std::cout << C[0][0] << "C[0][0]" << '\n';
	std::cout << P[0][0] << "P[0][0]" << '\n';
	infile.close();           
}


//Vertices transformation
class Vertices
{
public:
	float *NormalizedMatrix =new float[3];
	Vertices(float *a);
	~Vertices();
	void transform(float x,float y,float z, int al, int be, int th,float h,float d,float f);

private:
	float *list;
};

Vertices::Vertices(float *a)
{
	list = new float[4];
	for (int i = 0; i<=2; i++) {
	list[i] = a[i];
	}
	list[3] = 1;
	
}

Vertices::~Vertices()
{
	if (list)
		delete[]list;
	if (NormalizedMatrix)
		delete[]NormalizedMatrix;
}

void Vertices::transform (float x,float y,float z,int al,int be, int th,float h,float d, float f){
	float MviewMatrixs1[4] = { 0 }; // Matrix
	MviewMatrixs1[0] = list[0]-x;
	MviewMatrixs1[1] = list[1]-y;
	MviewMatrixs1[2] = list[2]-z;
	MviewMatrixs1[3] = 1;

	float MviewMatrixs2[4] = {0};
	MviewMatrixs2[0] = MviewMatrixs1[0] * cos(be*Pi/180)*cos(th*Pi/180) + MviewMatrixs1[1] *cos(be*Pi / 180) * sin(th*Pi/180)- MviewMatrixs1[2]*sin(be*Pi/180);
	MviewMatrixs2[1] = MviewMatrixs1[0] *(-cos(al*Pi / 180)*sin(th*Pi / 180)+ sin(al)*sin(be*Pi / 180)*cos(th*Pi / 180))
		               + MviewMatrixs1[1] * (cos(al*Pi / 180) * cos(be*Pi/180)+sin(al*Pi / 180)*sin(be*Pi / 180)*sin(th*Pi / 180))
		               + MviewMatrixs1[2] * (sin(be*Pi / 180)*cos(be*Pi / 180));
	MviewMatrixs2[2] = MviewMatrixs1[0] * (sin(al*Pi / 180)*sin(th*Pi / 180) + cos(al*Pi / 180)*sin(be*Pi / 180)*cos(th*Pi / 180))
		               + MviewMatrixs1[1] * (-sin(al*Pi / 180)*cos(be*Pi / 180)+cos(al*Pi / 180)*sin(be*Pi / 180)*sin(th*Pi / 180))
		               + MviewMatrixs1[2] * (cos(al*Pi / 180)*cos(be*Pi / 180));
	MviewMatrixs2[3] = 1;
	float MpersMatrixs[3] = {0};
	MpersMatrixs[0] = MviewMatrixs2[0] * d / h; 
	MpersMatrixs[1]= MviewMatrixs2[1] * d / h; 
	MpersMatrixs[2]= MviewMatrixs2[3] * f / (f - d) + MviewMatrixs2[3] * (-d * f / (f - d));
	
	*NormalizedMatrix = MpersMatrixs[0]/h;
	*(NormalizedMatrix+1) = MpersMatrixs[1]/h;
	*(NormalizedMatrix+2) = (MpersMatrixs[2] - d) / (f-d);
}   

class ZBuffer 
{
  public:

	Pixel IB[400][400];
	Pixel ZB[400][400];

	ZBuffer() {
		for (int i = 0; i < 400; i++) {
			for (int j = 0; j <400; j++)
			{
				ZB[i][j].z = -1;
				ZB[i][j].R = 0;
				ZB[i][j].G = 0;
				ZB[i][j].B = 0;
				IB[i][j].z = -1;
				IB[i][j].R = 0;
				IB[i][j].G = 0;
				IB[i][j].B = 0;
			}
		}
	}

	void OriginalImageBufferFresh(int OP[][4], int k)
	{
		int Za, Zb;
		float R = 2;
		float G = 3;
		float B = 3;

		for (int i = 0; i < k; i++)
		{

			int VA1 = OP[i][1]; int VA2 = OP[i][2]; int VA3 = OP[i][3];
			Line *line = CreateLines(&intfinal[VA1][0], &intfinal[VA2][0], &intfinal[VA3][0]);
			SortedEdgeTable *SET = CreatEdgeList(line, 3);
			ActiveEdgeTable *AET = Created_AET(SET);
			while (AET->head->next != NULL)
			{
				EdgeList *EL = AET->head;
				int b = 1;
				int y = 0;
				while (EL->next != NULL)
				{
					y = AET->y;
					if (b > 0)
					{
						int left = EL->X;
						int right = EL->next->X;

						if (!(EL->X - EL->next->X >= -DBL_EPSILON && EL->X - EL->next->X <= DBL_EPSILON))
						{
							if (!(EL->X - left >= -DBL_EPSILON && EL->X - left <= DBL_EPSILON))
							{
								left += 1;
							}
							if (!(EL->next->X - right >= -DBL_EPSILON && EL->next->X - right <= DBL_EPSILON))
							{
								right -= 1;
							}
						}

						for (int x = left; x < right; ++x)
						{
							Za = EL->Zmax - EL->DeltaZ*(EL->Ymax - y);
							Zb = EL->next->Zmax - EL->next->DeltaZ*(EL->next->Ymax - y);
							IB[x][y].z = Zb - (Zb - Za)*(right - x) / (left - right);
							IB[x][y].R = R;
							IB[x][y].G = G;
							IB[x][y].B = B;

							if (IB[x][y].z > ZB[y][x].z)//判断Z
							{
								ZB[x][y].z = IB[x][y].z;
								ZB[x][y].R = IB[x][y].R;
								ZB[x][y].G = IB[x][y].G;
								ZB[x][y].B = IB[x][y].B;
							}
						}
					}
				}
				EL = EL->next;
				b = -b;
				AET = Update_AET(AET, SET);
			}
		}
	}


	~ZBuffer() 
	{
	};
};

void init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
}

/*void myDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glRectf(-0.5f, -0.5f, 0.5f, 0.5f);
	glFlush();
}*/

void Display() //OpenGL Display call-back function
{
	
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.0, 0.4, 1);
	glLineWidth(1.0);
	
	for (int n = 0; n <2700; n++) {

		glBegin(GL_LINE_LOOP);
			
		   for (int w = 1; w <= 3; w++) {
				int series = OutputPolygon[n][w];  //Get the x and y of the final vertices
				glVertex2f((final[series][0]), (final[series][1])); //

		    }
		glEnd();
	}

	/*for (int x = 0; x < 800; x++)
	{
		for (int y = 0; y < 800; y++)
		{
			glBegin(GL_POINTS);
			glColor3f(ZB[x][y].R,ZB[x][y].G,ZB[x][y].B);
			glVertex2i(x,y);
		}

	}*/
	glFlush();
}


int main(int argc, char *argv[])
{
	//MallocZB();
	float WSCoordinate[5000][3] = {0};
	int OriginPolygon[5000][4] = {0};
	
	int Alpha, Beta, Theta;
	float x1, y1, z1, d1,h1,f1;

	readTxt("./queen.d.txt",WSCoordinate,OriginPolygon);
	std::cout << WSCoordinate[0][0] <<"Original Vertices"<<'\n';
	std::cout << OriginPolygon[0][0]<<"Original Polygons"<<'\n';
	
	int M=1;
	std::cout << "Choose the mode(1.Test mode 2.Final mode):";
	std::cin >> M;
	if (M ==1) 
	{
		x1 = 0; y1 = 0; z1 = 1;
		Alpha = 5; Beta = 5; Theta =15;
		d1 = 2; h1 = 4; f1 = 9;
	}
	else
	{
		std::cout << "Input the camera coordinate:";
		std::cin >> x1 >> y1 >> z1;
		std::cout << "Input the angle:";
		std::cin >> Alpha >> Beta >> Theta;
		std::cout << "Input the distance:";
		std::cin >> d1;
		std::cout << "Input the height:";
		std::cin >> h1;
		std::cout << "Input the F:";
		std::cin >> f1;
	}

	for (int t = 0; t <1380; t++) {
		
		Vertices vt1 ( WSCoordinate[t]);
		vt1.transform (x1,y1,z1,Alpha,Beta,Theta,h1,d1,f1);
		
		for (int r = 0; r < 3; r++) {
			final[t][r] = vt1.NormalizedMatrix[r];
		}
	}
	std::cout << final[400][2]<<'\n';

	for (int i = 0; i < 1380; i++)
	{
		intfinal[i][0] =floor( (final[i][0] + 1) * 400);
		intfinal[i][1] =floor ((final[i][1] + 1) * 400);
		intfinal[i][2] = final[i][2];
	}

	float z3 = 0;
	int Vtad0, Vtad1, Vtad2 = 0; int i3=0;
	float V1[3], V2[3] = { 0 };
	for (int k = 0; k <2751; k++) {
		
		Vtad0 = OriginPolygon[k][1];
		Vtad1 = OriginPolygon[k][2];
		Vtad2 = OriginPolygon[k][3];

		V1[0] = final[Vtad1][0] - final[Vtad0][0]; //V1 is the first vector of the polygon formed by two vertices
		V1[1] = final[Vtad1][1] - final[Vtad0][1];
		V1[2] = final[Vtad1][2] - final[Vtad0][2];

		V2[0] = final[Vtad2][0] - final[Vtad0][0]; //V2 is the second vector
		V2[1] = final[Vtad2][1] - final[Vtad0][1];
		V2[2] = final[Vtad2][2] - final[Vtad0][2];

		if (V1[0]*V2[1]-V1[1]*V2[0]>=0) //Caculating the Z-direction of the normal
		{
				for (int i2 = 0; i2 <= 3; i2++) {
					OutputPolygon[i3][i2] = OriginPolygon[k][i2];
				}
				i3++;
		}
		
	}
	
	std::cout << OutputPolygon[102][2] << '\t' << "OutputPolygons" << '\n';


	ZBuffer RandomZB;
	RandomZB.OriginalImageBufferFresh(OutputPolygon,2500);
	
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(400, 400);
	glutCreateWindow("Lab 1");
	init();
	
	

	glutDisplayFunc(&Display);
		/*for (int n = 0; n < sizeof OutputPolygon; n++) {
			glBegin(GL_LINES);
			for (int w = 0; w < 2; w++) {
				int series = OutputPolygon[n][w];
				glVertex2f(final[series][1], final[series][2]);
			}
			glEnd();
		}
	}*/
	glutMainLoop();
	return 0;
}

