#include "t3f/t3f.h"

#define _T3LOGO_MAX_VERTICES 64

#define _T3LOGO_TARGET_TILT  (0.5)
#define _T3LOGO_TARGET_ANGLE (ALLEGRO_PI * 2.25)
#define _T3LOGO_LIGHT_ANGLE  (ALLEGRO_PI * 1.25)

/* shape constructed of triplets of vertices */
typedef struct
{

	ALLEGRO_VERTEX vertex[_T3LOGO_MAX_VERTICES];
	int vertex_count;

	ALLEGRO_VERTEX transformed_vertex[_T3LOGO_MAX_VERTICES];
	float z_depth;
	float angle;

} SHAPE;

/* structure to hold all of our app-specific data */
typedef struct
{

	SHAPE logo_side[3];
	SHAPE * logo_side_zsort[3];
	float angle;
	float tilt;
	float v_tilt;   // tilt velocity
	float vf_tilt;  // tilt velocity friction
	float v_angle;  // angle velocity
	float vf_angle; // angle velocity friction
	int tick;

} APP_INSTANCE;

static int side_z_depth_qsort_proc(const void * p1, const void * p2)
{
	SHAPE * side1 = *(SHAPE **)p1;
	SHAPE * side2 = *(SHAPE **)p2;

	return side2->z_depth < side1->z_depth;
}

static void init_shape(SHAPE * sp)
{
	memset(sp, 0, sizeof(SHAPE));
}

static void add_vertex(SHAPE * sp, float x, float y, float z)
{
	sp->vertex[sp->vertex_count].x = x;
	sp->vertex[sp->vertex_count].y = y;
	sp->vertex[sp->vertex_count].z = z;
	sp->vertex_count++;
}

static float linear_spread(float angle_1, float angle_2, float val_min, float val_max)
{
	float d = fabs(val_max - val_min);
	float d_angle = angle_2 - angle_1;

	if(d_angle < 0)
	{
		d_angle += ALLEGRO_PI;
	}
	d_angle = fmod(d_angle, ALLEGRO_PI); // loop at ALLEGRO_PI intervals
	d_angle -= ALLEGRO_PI / 2.0; // subtract 90 degrees
	if(d_angle < 0)
	{
		return val_min - d * (d_angle / (ALLEGRO_PI / 2.0));
	}
	return val_min + d * (d_angle / (ALLEGRO_PI / 2.0));
}

static ALLEGRO_COLOR get_lit_color(ALLEGRO_COLOR base_color, float light_angle, float side_normal_angle)
{
	float r, g, b;
	float h, s, l;
	float target_l;

	al_unmap_rgb_f(base_color, &r, &g, &b);
	al_color_rgb_to_hsl(r, g, b, &h, &s, &l);
	target_l = linear_spread(light_angle, side_normal_angle, l * 0.75, l);
//	target_l = (1.0 - fmod(fabs(light_angle - side_normal_angle), ALLEGRO_PI) / ALLEGRO_PI) * l;

	return al_color_hsl(h, s, target_l);
}

static void set_shape_orientation(SHAPE * sp, float ox, float oy, float angle, float tilt, float scale)
{
	int i;
	float vangle;
	float vdistance;
	float temp_z;

	sp->z_depth = 1000000;
	for(i = 0; i < sp->vertex_count; i++)
	{
		vangle = atan2(sp->vertex[i].z, sp->vertex[i].x);
		vdistance = hypot(sp->vertex[i].x, sp->vertex[i].z);
		temp_z = sin(vangle + angle) * vdistance;
		if(temp_z < sp->z_depth)
		{
			sp->z_depth = temp_z;
		}
		sp->transformed_vertex[i].x = ox + cos(vangle + angle) * vdistance * scale;
		sp->transformed_vertex[i].y = oy + (sp->vertex[i].y + temp_z * tilt) * scale;
		sp->transformed_vertex[i].z = 0;
		sp->transformed_vertex[i].color = get_lit_color(al_map_rgb(46, 104, 158), _T3LOGO_LIGHT_ANGLE, sp->angle);
	}
}

static void render_shape(SHAPE * sp)
{
	al_draw_prim(sp->transformed_vertex, NULL, NULL, 0, sp->vertex_count + 1, ALLEGRO_PRIM_TRIANGLE_LIST);
}

static float get_punch_velocity(float start_angle, float end_angle, float friction)
{
	float current_angle = end_angle;
	float v = 0.0;

	while(current_angle > start_angle)
	{
		v += friction;
		current_angle -= v;
	}
	return v;
}

static float get_punch_tilt(float start_tilt, float end_tilt, float friction)
{
	float current_tilt = end_tilt;
	float v = 0.0;

	while(current_tilt > start_tilt)
	{
		v += friction;
		current_tilt -= v;
	}
	return v;
}

static float get_acceleration(float distance, float t)
{
	return (2.0 * distance) / (t * t);
}

/* main logic routine */
void app_logic(void * data)
{
	APP_INSTANCE * app = (APP_INSTANCE *)data;

	if(t3f_key[ALLEGRO_KEY_SPACE])
	{
		app->tick = 60;
		app->angle = 0;
		app->vf_angle = -get_acceleration(_T3LOGO_TARGET_ANGLE, app->tick);
		app->v_angle = get_punch_velocity(0, _T3LOGO_TARGET_ANGLE, -app->vf_angle);
		app->tilt = 0;
		app->vf_tilt = -get_acceleration(_T3LOGO_TARGET_TILT, app->tick);
		app->v_tilt = get_punch_tilt(0.0, _T3LOGO_TARGET_TILT, -app->vf_tilt);
		t3f_key[ALLEGRO_KEY_SPACE] = 0;
	}
	if(app->tick > -8)
	{
		app->tilt += app->v_tilt;
		app->v_tilt += app->vf_tilt;
		app->angle += app->v_angle;
		app->v_angle += app->vf_angle;
		app->tick--;
	}
	else if(app->tick == -8)
	{
		app->tilt = _T3LOGO_TARGET_TILT;
		app->angle = _T3LOGO_TARGET_ANGLE;
		app->tick--;
	}

	/* manual controls */
	if(t3f_key[ALLEGRO_KEY_LEFT])
	{
		app->angle -= ALLEGRO_PI / 16.0;
		printf("angle %f\n", app->angle);
		t3f_key[ALLEGRO_KEY_LEFT] = 0;
	}
	if(t3f_key[ALLEGRO_KEY_RIGHT])
	{
		app->angle += ALLEGRO_PI / 16.0;
		printf("angle %f\n", app->angle);
		t3f_key[ALLEGRO_KEY_RIGHT] = 0;
	}
	if(t3f_key[ALLEGRO_KEY_UP])
	{
		app->tilt -= 0.1;
		printf("tilt %f\n", app->tilt);
		t3f_key[ALLEGRO_KEY_UP] = 0;
	}
	if(t3f_key[ALLEGRO_KEY_DOWN])
	{
		app->tilt += 0.1;
		printf("tilt %f\n", app->tilt);
		t3f_key[ALLEGRO_KEY_DOWN] = 0;
	}
	set_shape_orientation(&app->logo_side[0], 256, 256, app->angle, app->tilt, 48);
	app->logo_side[0].angle = app->angle + ALLEGRO_PI / 2.0;
	set_shape_orientation(&app->logo_side[1], 256, 256, app->angle, app->tilt, 48);
	app->logo_side[1].angle = app->angle + ALLEGRO_PI;
	set_shape_orientation(&app->logo_side[2], 256, 256, app->angle, app->tilt, 48);
	app->logo_side[2].angle = app->angle + ALLEGRO_PI * 1.5;
	qsort(app->logo_side_zsort, 3, sizeof(SHAPE *), side_z_depth_qsort_proc);
}

/* main rendering routine */
void app_render(void * data)
{
	APP_INSTANCE * app = (APP_INSTANCE *)data;

	al_clear_to_color(t3f_color_white);
	render_shape(app->logo_side_zsort[0]);
	render_shape(app->logo_side_zsort[1]);
	render_shape(app->logo_side_zsort[2]);
}

/* initialize our app, load graphics, etc. */
bool app_initialize(APP_INSTANCE * app, int argc, char * argv[])
{
	/* initialize T3F */
	if(!t3f_initialize(T3F_APP_TITLE, 640, 480, 60.0, app_logic, app_render, T3F_DEFAULT, app))
	{
		printf("Error initializing T3F\n");
		return false;
	}
	memset(app, 0, sizeof(APP_INSTANCE));
	init_shape(&app->logo_side[0]);
	add_vertex(&app->logo_side[0], -3, -3, -3);
	add_vertex(&app->logo_side[0], 3, -3, -3);
	add_vertex(&app->logo_side[0], -3, -1, -3);
	add_vertex(&app->logo_side[0], 3, -3, -3);
	add_vertex(&app->logo_side[0], 3, -1, -3);
	add_vertex(&app->logo_side[0], -3, -1, -3);
	add_vertex(&app->logo_side[0], -1, -1, -3);
	add_vertex(&app->logo_side[0], 1, -1, -3);
	add_vertex(&app->logo_side[0], -1, 3, -3);
	add_vertex(&app->logo_side[0], 1, -1, -3);
	add_vertex(&app->logo_side[0], 1, 3, -3);
	add_vertex(&app->logo_side[0], -1, 3, -3);

	init_shape(&app->logo_side[1]);
	add_vertex(&app->logo_side[1], 3, -3, -3);
	add_vertex(&app->logo_side[1], 3, -3, 3);
	add_vertex(&app->logo_side[1], 3, -1, -3);
	add_vertex(&app->logo_side[1], 3, -3, 3);
	add_vertex(&app->logo_side[1], 3, -1, 3);
	add_vertex(&app->logo_side[1], 3, -1, -3);
	add_vertex(&app->logo_side[1], 3, -1, -1);
	add_vertex(&app->logo_side[1], 3, -1, 1);
	add_vertex(&app->logo_side[1], 3, 3, -1);
	add_vertex(&app->logo_side[1], 3, -1, 1);
	add_vertex(&app->logo_side[1], 3, 3, 1);
	add_vertex(&app->logo_side[1], 3, 3, -1);

	init_shape(&app->logo_side[2]);
	add_vertex(&app->logo_side[2], -3, -3, 3);
	add_vertex(&app->logo_side[2], 3, -3, 3);
	add_vertex(&app->logo_side[2], -3, -1, 3);
	add_vertex(&app->logo_side[2], 3, -3, 3);
	add_vertex(&app->logo_side[2], 3, -1, 3);
	add_vertex(&app->logo_side[2], -3, -1, 3);
	add_vertex(&app->logo_side[2], -1, -1, 3);
	add_vertex(&app->logo_side[2], 1, -1, 3);
	add_vertex(&app->logo_side[2], -1, 3, 3);
	add_vertex(&app->logo_side[2], 1, -1, 3);
	add_vertex(&app->logo_side[2], 1, 3, 3);
	add_vertex(&app->logo_side[2], -1, 3, 3);

	app->logo_side_zsort[0] = &app->logo_side[0];
	app->logo_side_zsort[1] = &app->logo_side[1];
	app->logo_side_zsort[2] = &app->logo_side[2];

	app->tick = -1000;

	return true;
}

int main(int argc, char * argv[])
{
	APP_INSTANCE app;

	if(app_initialize(&app, argc, argv))
	{
		t3f_run();
	}
	else
	{
		printf("Error: could not initialize T3F!\n");
	}
	t3f_finish();
	return 0;
}
