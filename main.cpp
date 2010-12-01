#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>

#define SCREEN_WIDTH	1000
#define SCREEN_HEIGHT	600
#define SCREEN_BPP		32
#define PI 3.14159265358979
#define GRAVITY 0.000005
#define FPS false
#define DEBUG exit(__LINE__);
#define FRAMERATE 80

#define sina(a) (sin((a)*PI/180))
#define cosa(a) (cos((a)*PI/180))

/* KARTAN DATAN ARVOJA
*
*  1 = seinä
*  2 = laava
*  3 = spawn
*  4 = maali
*  5 =
*
*/

struct camera;

namespace map {
	const int width=100, height=100;
	float size = 1;
	int data[width][height] = {{0}};
	int unitval[10][2] = {{0}};

	void generate(int gx, int gz, int sx = width/2, int sz = height/2) {
		unitval[3] = {sx, sz};
		unitval[4] = {gx, gz};

		for (int x = 0; x < width; x++) for (int z = 0; z < height; z++) {
			if (x == sx && z == sz) data[x][z] = 3;
			if (x == gx && z == gz) data[x][z] = 4;
			else data[x][z] = ((x%2)*(z%2));
		}
		for (int x = 0; x < width; x++) {
			data[x][0] = 2;
			data[x][height-1] = 2;
		}
		for (int z = 0; z < height; z++) {
			data[0][z] = 2;
			data[width-1][z] = 2;
		}
	}
};
namespace move {
	float rotax, rotay, rotaz, x, y, z;
	int mx, my;
	int blockx() { return (int)((move::x/map::size+map::width/2)+0.5f); };
	int blockz() { return (int)((move::z/map::size+map::height/2)+0.5f); };
	int blockx(float s) { return (int)((s/map::size+map::width/2)+0.5f); };
	int blockz(float s) { return (int)((s/map::size+map::height/2)+0.5f); };
}
namespace cam { float rotax, rotay, rotaz, zoom, x, y, z; }
namespace fps { float fps; int frames = 0, T0 = 0;}
namespace clock {
	int time, starttime, count = -1, ret;
	int start(int c, int ret) { count = c; starttime = SDL_GetTicks(); };
	int update() {
		if ( count != -1 ) {
			time = SDL_GetTicks();
			if ((time-starttime)/1000>=count) return ret;
		}
		return 0;
	};
};

int		videoFlags;
int		running = 0;
const	SDL_VideoInfo *videoInfo;
SDL_Surface *surface;
SDL_Event event;
Uint8	*napit;
GLuint tex_floor, tex_wall, tex_roof;
std::vector<camera> cameras;

struct camera {
	camera(int px, int py) {
		x = px;
		z = py;
		a = rand();
		r = ((rand()%2)?(1):(-1));

	}
	int x, z, a, r;
	float period() { return sina(a); };
	void update() {
		a+=r;
	};
};

int main( int argc, char **argv ) {
	SDL_Surface* img;
	fps::frames = 0;

	atexit(SDL_Quit);
	srand(SDL_GetTicks());

    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) exit(-1);
    videoInfo = SDL_GetVideoInfo();
    if ( !videoInfo ) exit(-2);

    videoFlags  = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE | SDL_RESIZABLE | SDL_HWSURFACE;
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    surface = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, videoFlags );
    if ( !surface ) exit(-3);

	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_shininess[] = { 50.0 };
	GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
	glClearColor (0.0, 0.0, 0.0, 0.0);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glShadeModel( GL_SMOOTH );
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClearDepth( 1.0f );
    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_CULL_FACE );
//	glEnable( GL_LIGHTING );
	glEnable( GL_LIGHT0 );
	glDepthFunc( GL_LEQUAL );
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

    GLfloat ratio = ( GLfloat )SCREEN_WIDTH / ( GLfloat )SCREEN_HEIGHT;
    glViewport( 0, 0, ( GLsizei )SCREEN_WIDTH, ( GLsizei )SCREEN_HEIGHT );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	gluPerspective( 120.0f, ratio, 0.1f, 90.0f );
    glMatrixMode( GL_MODELVIEW );

    glLoadIdentity();

//	GLfloat tmp[4] = {1,1,1,1};
 //   glLightfv( GL_LIGHT0, GL_SPECULAR, tmp );
//    delete tmp;


	// TEKSTUURIT


	glGenTextures(1, &tex_wall); glBindTexture(GL_TEXTURE_2D, tex_wall);
	img = SDL_DisplayFormat(SDL_LoadBMP("wall.bmp"));
//	gluBuild2DMipmaps(GL_TEXTURE_2D, img->format->BytesPerPixel, img->w, img->h, GL_BGRA, GL_UNSIGNED_BYTE, img->pixels);
	glTexImage2D(GL_TEXTURE_2D, 0, img->format->BytesPerPixel, img->w, img->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, img->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &tex_floor); glBindTexture(GL_TEXTURE_2D, tex_floor);
	img = SDL_DisplayFormat(SDL_LoadBMP("floor.bmp"));
	glTexImage2D(GL_TEXTURE_2D, 0, img->format->BytesPerPixel, img->w, img->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, img->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &tex_roof); glBindTexture(GL_TEXTURE_2D, tex_roof);
	img = SDL_DisplayFormat(SDL_LoadBMP("roof.bmp"));
	glTexImage2D(GL_TEXTURE_2D, 0, img->format->BytesPerPixel, img->w, img->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, img->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Asetuksien määrittelyä
	map::generate(rand()%map::width, rand()%map::height);

	move::y		= -1;
//	move::rotay	= 20;
	move::rotax	= 40;
	for(int x = 0; x < map::width; x++)
	for(int z = 0; z < map::height; z++) {
		if (map::data[x][z]==3) {
			move::x		= map::size*(x-map::width/2);
			move::z		= map::size*(z-map::height/2);
		}
	}

	for(int i = 0; i < 1500; i++) {
		int tx = rand()%(map::width/2), ty = rand()%(map::height/2);
		cameras.push_back(camera(tx*2,ty*2));
	}

	//Listojen luominen
	int map_list = glGenLists(1);
	glNewList(map_list, GL_COMPILE);
		glBindTexture(GL_TEXTURE_2D, tex_floor);
		glBegin( GL_QUADS );
			for(int x = 0; x < map::width; x++)
			for(int z = 0; z < map::height; z++) {
				if (map::data[x][z]!=1) {
					glNormal3f(1, 0, 0);
					if (map::data[x][z]==2) glColor3f(1,0,0);
					glTexCoord2f(2.0f,2.0f); glVertex3f(((x-map::width/2)*map::size)-map::size/2, 0.0f,((z-map::height/2)*map::size)+map::size/2);
					glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+map::size/2, 0.0f,((z-map::height/2)*map::size)+map::size/2);
					glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+map::size/2, 0.0f,((z-map::height/2)*map::size)-map::size/2);
					glTexCoord2f(0.0f,2.0f); glVertex3f(((x-map::width/2)*map::size)-map::size/2, 0.0f,((z-map::height/2)*map::size)-map::size/2);
					glColor3f(1,1,1);
				}
			}
		glEnd();
		glBindTexture(GL_TEXTURE_2D, tex_wall);
		glBegin( GL_QUADS );
			for(int x = 0; x < map::width; x++)
			for(int z = 0; z < map::height; z++){
				if (map::data[x][z]!=1) {
					if (map::data[x-1][z]==1 || x == 0) { //VASEN
						glNormal3f(-1, 0, 0);
						glTexCoord2f(2.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/2), 0.0f,((z-map::height/2)*map::size)+(map::size/2));
						glTexCoord2f(0.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/2), 0.0f,((z-map::height/2)*map::size)-(map::size/2));
						glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/2), (map::size/2),((z-map::height/2)*map::size)-(map::size/2));
						glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/2), (map::size/2),((z-map::height/2)*map::size)+(map::size/2));
					}
					if (map::data[x][z-1]==1 || z == 0) { //TAKA
						glNormal3f(0, 0, -1);
						glTexCoord2f(2.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/2), 0.0f,((z-map::height/2)*map::size)-(map::size/2));
						glTexCoord2f(0.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/2), 0.0f,((z-map::height/2)*map::size)-(map::size/2));
						glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/2), (map::size/2),((z-map::height/2)*map::size)-(map::size/2));
						glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/2), (map::size/2),((z-map::height/2)*map::size)-(map::size/2));
					}
					if (map::data[x+1][z]==1 || x == map::width-1) { //OIKEA
						glNormal3f(1, 0, 0);
						glTexCoord2f(2.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/2), 0.0f,((z-map::height/2)*map::size)+(map::size/2));
						glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/2), (map::size/2),((z-map::height/2)*map::size)+(map::size/2));
						glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/2), (map::size/2),((z-map::height/2)*map::size)-(map::size/2));
						glTexCoord2f(0.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/2), 0.0f,((z-map::height/2)*map::size)-(map::size/2));
					}
					if (map::data[x][z+1]==1 || z == map::height-1) { //ETU
						glNormal3f(0, 0, 1);
						glTexCoord2f(2.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/2), 0.0f,((z-map::height/2)*map::size)+(map::size/2));
						glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/2), (map::size/2),((z-map::height/2)*map::size)+(map::size/2));
						glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/2), (map::size/2),((z-map::height/2)*map::size)+(map::size/2));
						glTexCoord2f(0.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/2), 0.0f,((z-map::height/2)*map::size)+(map::size/2));
					}
				}
			}
		glEnd();
		glBindTexture(GL_TEXTURE_2D, tex_roof);
		glBegin( GL_QUADS );
			for(int x = 0; x < map::width; x++)
			for(int z = 0; z < map::height; z++)
				if (map::data[x][z]!=1) {
					glNormal3f(1, 0, 0);
					glTexCoord2f(2.0f,2.0f); glVertex3f(((x-map::width/2)*map::size)-map::size/2, map::size/2,((z-map::height/2)*map::size)+map::size/2);
					glTexCoord2f(0.0f,2.0f); glVertex3f(((x-map::width/2)*map::size)-map::size/2, map::size/2,((z-map::height/2)*map::size)-map::size/2);
					glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+map::size/2, map::size/2,((z-map::height/2)*map::size)-map::size/2);
					glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+map::size/2, map::size/2,((z-map::height/2)*map::size)+map::size/2);
				}
		glEnd();
	glEndList();
	int pointer_list = glGenLists(1);
	glNewList(pointer_list, GL_COMPILE);
		glBegin( GL_TRIANGLES ); // SPAWN & MAALI
			glColor4f(0, 1, 0, .5);
			glVertex3f(((map::unitval[4][0]-map::width/2)*map::size)-map::size/2, 0.0f,((map::unitval[4][1]-map::height/2)*map::size)-map::size/2);
			glVertex3f(((map::unitval[4][0]-map::width/2)*map::size), 0.5f,((map::unitval[4][1]-map::height/2)*map::size));
			glVertex3f(((map::unitval[4][0]-map::width/2)*map::size)+map::size/2, 0.0f,((map::unitval[4][1]-map::height/2)*map::size)+map::size/2);
			glVertex3f(((map::unitval[4][0]-map::width/2)*map::size)+map::size/2, 0.0f,((map::unitval[4][1]-map::height/2)*map::size)-map::size/2);
			glVertex3f(((map::unitval[4][0]-map::width/2)*map::size), 0.5f,((map::unitval[4][1]-map::height/2)*map::size));
			glVertex3f(((map::unitval[4][0]-map::width/2)*map::size)-map::size/2, 0.0f,((map::unitval[4][1]-map::height/2)*map::size)+map::size/2);
			glColor3f(1, 1, 1);
		glEnd();
	glEndList();
	int camera_list = glGenLists(1);
	glNewList(camera_list, GL_COMPILE);
		glBegin( GL_QUADS ); // KAMERALAATIKKO
		for(int i = 0; i < cameras.size(); i++) {
			int x = cameras[i].x;
			int z = cameras[i].z;
			glTexCoord2f(2.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/20), map::size/2-map::size/20,((z-map::height/2)*map::size)+(map::size/20));
			glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/20), (map::size/2),((z-map::height/2)*map::size)+(map::size/20));
			glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/20), (map::size/2),((z-map::height/2)*map::size)-(map::size/20));
			glTexCoord2f(0.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/20), map::size/2-map::size/20,((z-map::height/2)*map::size)-(map::size/20));

			glTexCoord2f(2.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/20), map::size/2-map::size/20,((z-map::height/2)*map::size)-(map::size/20));
			glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/20), (map::size/2),((z-map::height/2)*map::size)-(map::size/20));
			glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/20), (map::size/2),((z-map::height/2)*map::size)-(map::size/20));
			glTexCoord2f(0.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/20), map::size/2-map::size/20,((z-map::height/2)*map::size)-(map::size/20));

			glTexCoord2f(2.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/20), map::size/2-map::size/20,((z-map::height/2)*map::size)+(map::size/20));
			glTexCoord2f(0.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/20), map::size/2-map::size/20,((z-map::height/2)*map::size)-(map::size/20));
			glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/20), (map::size/2),((z-map::height/2)*map::size)-(map::size/20));
			glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/20), (map::size/2),((z-map::height/2)*map::size)+(map::size/20));

			glTexCoord2f(2.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/20), map::size/2-map::size/20,((z-map::height/2)*map::size)+(map::size/20));
			glTexCoord2f(0.0f,1.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/20), map::size/2-map::size/20,((z-map::height/2)*map::size)+(map::size/20));
			glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+(map::size/20), (map::size/2),((z-map::height/2)*map::size)+(map::size/20));
			glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)-(map::size/20), (map::size/2),((z-map::height/2)*map::size)+(map::size/20));

			glTexCoord2f(2.0f,2.0f); glVertex3f(((x-map::width/2)*map::size)-map::size/20, map::size/2-map::size/20,((z-map::height/2)*map::size)+map::size/20);
			glTexCoord2f(0.0f,2.0f); glVertex3f(((x-map::width/2)*map::size)-map::size/20, map::size/2-map::size/20,((z-map::height/2)*map::size)-map::size/20);
			glTexCoord2f(0.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+map::size/20, map::size/2-map::size/20,((z-map::height/2)*map::size)-map::size/20);
			glTexCoord2f(2.0f,0.0f); glVertex3f(((x-map::width/2)*map::size)+map::size/20, map::size/2-map::size/20,((z-map::height/2)*map::size)+map::size/20);
		}
		glEnd();
	glEndList();

	int startTimer, endTimer, deltaTime;

    for (int frm = 0; !running; frm++) {
		startTimer = SDL_GetTicks();

		while ( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
			case SDL_KEYDOWN:
				switch ( event.key.keysym.sym ) {
				case SDLK_ESCAPE:
					running = 1;
					break;
				case SDLK_F1:
					break;
				default:
					break;
				}
				break;
			case SDL_QUIT:
				running = 1;
				break;
			default:
				break;
			}
		}

    	// Syötteiden käsittely
		napit = SDL_GetKeyState(NULL);

		if (napit[SDLK_LEFT]) move::rotay-=(1.5+napit[SDLK_LSHIFT]);
		if (napit[SDLK_RIGHT]) move::rotay+=(1.5+napit[SDLK_LSHIFT]);
		if (napit[SDLK_UP]) move::rotax-=(.5+napit[SDLK_LSHIFT]);
		if (napit[SDLK_DOWN]) move::rotax+=(.5+napit[SDLK_LSHIFT]);

		float oldx = move::x;
		float oldz = move::z;

		if (napit[SDLK_w]) {
			move::x+=(napit[SDLK_LSHIFT]*cosa(move::rotay)+(float)cosa(move::rotay))/100.0;
			move::z+=(napit[SDLK_LSHIFT]*sina(move::rotay)+(float)sina(move::rotay))/100.0;
		} else if (napit[SDLK_s]) {
			move::x-=(napit[SDLK_LSHIFT]*cosa(move::rotay)+(float)cosa(move::rotay))/100.0;
			move::z-=(napit[SDLK_LSHIFT]*sina(move::rotay)+(float)sina(move::rotay))/100.0;
		}

		if (napit[SDLK_d]) {
			move::x+=1.4*(float)cosa(move::rotay+90)/100.0;
			move::z+=1.4*(float)sina(move::rotay+90)/100.0;
		} else if (napit[SDLK_a]) {
			move::x-=1.4*(float)cosa(move::rotay+90)/100.0;
			move::z-=1.4*(float)sina(move::rotay+90)/100.0;
		}
		if (map::data[move::blockx()][move::blockz()]==2) { // laava
			running = 2;
		} else if (map::data[move::blockx()][move::blockz()]==4) { // voitto!
			running = 3;
		} else if (map::data[move::blockx()][move::blockz()]==1) { // Törmäystarkistus
			if (abs(move::blockx()-move::blockx(oldx)) == 1) oldz = move::z;
			if (abs(move::blockz()-move::blockz(oldz)) == 1) oldx = move::x;
			move::x = oldx;
			move::z = oldz;
		}


		for(int i = 0; i < cameras.size(); i++) {
			if (abs(move::blockx()-cameras[i].x)<3 && abs(move::blockz()-cameras[i].z)<3) {
				if (sqrtf(pow((move::x-(((cameras[i].x-map::width/2)*map::size)+cosa(cameras[i].a))),2) + pow((move::z-(((cameras[i].z-map::width/2)*map::size)+sina(cameras[i].a))),2))<=.5) running = 4;
			}
		}


		//Piirtäminen
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); //Puhdistetaan puskurit
		glLoadIdentity(); //Alustetaan siirrot
//		gluLookAt(move::x - cosa(move::rotay), .3f, move::z - sina(move::rotay), move::x + 1000*cosa(move::rotay), .3f, move::z + 1000*sina(move::rotay), 0.0f, 1.0f, 0.0f); //Asetetaam näkymä/kameran sijainti
		gluLookAt(move::x , .3f, move::z , move::x + 1000*cosa(move::rotay), .3f, move::z + 1000*sina(move::rotay), 0.0f, 1.0f, 0.0f); //Asetetaam näkymä/kameran sijainti
		glCallList(map_list); // Kutsutaan GL:n listaa, joka piirtää kartan, seinät ja lattiat
		glCallList(camera_list);
		glDisable( GL_CULL_FACE );
		for(int i = 0; i < cameras.size(); i++) {
			glBegin( GL_TRIANGLES ); // KAMERA
				cameras[i].update();
				int x = cameras[i].x;
				int z = cameras[i].z;
				int a = cameras[i].a;
				glColor4f(1.0f,0.0f,0.0f, 1.0f);
				for (int v = 0, si=15; v < 360; v+=si){
					glVertex3f(((x-map::width/2)*map::size), map::size/2-map::size/20,((z-map::height/2)*map::size));
					glVertex3f(((x-map::width/2)*map::size)+cosa(a)+cosa(v)/2, 0,((z-map::height/2)*map::size)+sina(a)+sina(v)/2);
					glVertex3f(((x-map::width/2)*map::size)+cosa(a)+cosa(v+si)/2, 0,((z-map::height/2)*map::size)+sina(a)+sina(v+si)/2);
				}
				glColor3f(1,1,1);
			glEnd();
		}

		glDisable( GL_DEPTH_TEST );
		glCallList(pointer_list); // Kutsutaan GL:n listaa, joka piirtää apupisteet

		glEnable( GL_CULL_FACE );
		glEnable( GL_DEPTH_TEST );
		glBegin( GL_QUADS ); // KAMERA
			glColor3d(1,1,1);
			glVertex3f(move::x-.1, .01, move::z-.1);
			glVertex3f(move::x-.1, .01, move::z+.1);
			glVertex3f(move::x+.1, .01, move::z+.1);
			glVertex3f(move::x+.1, .01, move::z-.1);
		glEnd();
		SDL_GL_SwapBuffers( );

		printf("x=%i, y=%i, id=%i, c=%f, ", move::blockx(), move::blockz(), map::data[move::blockx()][move::blockz()], (clock::time-clock::starttime)/1000); //Tulostetaan tietoa sijainnista jms. DEBUG
		printf("rota=%f, rota_move=%f\n", move::rotay, atan2f((move::y + 100000*sina(move::rotay)),(move::x + 100000*cosa(move::rotay)))*180/PI);

		//aikalaskuri
		if ( running==0 ) running = clock::update();

		//FPS-limiter
		endTimer = SDL_GetTicks();
		deltaTime = endTimer - startTimer;
		if ( deltaTime < ( 1000 / FRAMERATE )) SDL_Delay( ( 1000 / FRAMERATE ) - deltaTime );
	}
	printf("x=%f\ny=%f\nz=%f\nrotx=%f\nroty=%f\nrotz=%f\nFPS=%f", move::x, move::y, move::z, move::rotax, move::rotay, move::rotaz, fps::fps);
	printf("\n");
	switch (running) {
		case 0:
			printf("ftw????\n");
			break;
		case 1:
			printf("Poistuttu turvallisesti\n");
			break;
		case 2:
			printf("Game Over! - Laava\n");
			break;
		case 3:
			printf("VOITTO!\n");
			break;
		case 4:
			printf("Game Over! - Kamera!\n");
			break;
	}
	exit(0);
	return( 0 );
}


