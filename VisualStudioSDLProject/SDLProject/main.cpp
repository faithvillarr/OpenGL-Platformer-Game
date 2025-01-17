#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define LEVEL1_WIDTH 25
#define LEVEL1_HEIGHT 8
#define LEVEL1_LEFT_EDGE 5.0f
#define NUM_OF_LIVES 3

#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <string>
#include "Entity.h"
#include "Map.h"
#include "Utility.h"
#include "Scene.h"
#include "MenuScreen.h"
#include "LevelA.h"
#include "LevelB.h"
#include "LevelC.h"
#include "Effects.h"

// ––––– CONSTANTS ––––– //
const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 168.0 / 255.0,
BG_GREEN =           220.0 / 255.0,
BG_BLUE =            255.0 / 255.0,
BG_OPACITY = 1.0f;
//168, 210, 255

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;


// ––––– GLOBAL VARIABLES ––––– //
Scene* g_current_scene;
MenuScreen* g_menu;
LevelA* g_levelA;
LevelB* g_levelB;
LevelC* g_levelC;
Entity* pause_menu;

Effects* g_effects;
Scene* g_levels[4];

int lives_left = 3;
bool g_game_won = false; 
bool g_paused = false;
Entity* lives;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

bool g_is_colliding_bottom = false;

// ––––– GENERAL FUNCTIONS ––––– //
void switch_to_scene(Scene* scene)
{
    g_current_scene = scene;
    g_current_scene->initialise(); // DON'T FORGET THIS STEP!
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Platformer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.Load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.SetProjectionMatrix(g_projection_matrix);
    g_shader_program.SetViewMatrix(g_view_matrix);

    glUseProgram(g_shader_program.programID);

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_levelA = new LevelA();
    g_levelB = new LevelB();
    g_levelC = new LevelC();
    g_menu = new MenuScreen();

    g_levels[0] = g_menu;
    g_levels[1] = g_levelA;
    g_levels[2] = g_levelB;
    g_levels[3] = g_levelC;

    // Start at level A
    switch_to_scene(g_levels[0]);

    g_effects = new Effects(g_projection_matrix, g_view_matrix);


    // -- LIVES -- //
    lives = new Entity[NUM_OF_LIVES];
    for (size_t i = 0; i < NUM_OF_LIVES; i++)
    {
        lives[i].set_entity_type(LIVES);
        lives[i].set_position(glm::vec3(0.5f +  0.75 * i, -0.5f, 0.0f));
        lives[i].set_movement(glm::vec3(0.0f));
        lives[i].m_speed = 0.0f;
        lives[i].set_acceleration(glm::vec3(0.0f));
        lives[i].m_texture_id = Utility::load_texture("assets/heart.png");
        lives[i].m_animation_index = NULL;
    }

    // -- PAUSE POP UP -- //
    pause_menu = new Entity();
    pause_menu->set_entity_type(LIVES);
    pause_menu->set_position(glm::vec3(0.0f));
    pause_menu->m_texture_id = Utility::load_texture("assets/pause-menu.png");
    pause_menu->m_animation_indices = NULL;

}

void process_input()
{

    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_current_scene->m_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            case SDLK_r:
                switch_to_scene(g_current_scene);
                break;
            case SDLK_p:
                if (g_current_scene != g_menu)
                    g_paused = !g_paused;
                    //set all movement to zero
                break;
            case SDLK_RETURN:
                if (g_current_scene == g_menu) {
                    switch_to_scene(g_levelA);
                    g_effects->start(SHRINK, 5.0f);
                }
                break;

            case SDLK_SPACE:
                // Jump
                if (g_current_scene->m_state.player->m_collided_bottom)
                {
                    g_current_scene->m_state.player->m_is_jumping = true;
                    Mix_PlayChannel(-1, g_current_scene->m_state.jump_sfx, 0);
                }
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    if (lives_left != 0 && !g_paused) {
        if (key_state[SDL_SCANCODE_LEFT])
        {
            g_current_scene->m_state.player->m_movement.x = -1.0f;
            g_current_scene->m_state.player->m_animation_indices = g_current_scene->m_state.player->m_walking[g_current_scene->m_state.player->LEFT];
        }
        else if (key_state[SDL_SCANCODE_RIGHT])
        {
            g_current_scene->m_state.player->m_movement.x = 1.0f;
            g_current_scene->m_state.player->m_animation_indices = g_current_scene->m_state.player->m_walking[g_current_scene->m_state.player->RIGHT];
        }

        if (glm::length(g_current_scene->m_state.player->m_movement) > 1.0f)
        {
            g_current_scene->m_state.player->m_movement = glm::normalize(g_current_scene->m_state.player->m_movement);
        }
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP) {
        if(!g_paused)
            g_current_scene->update(FIXED_TIMESTEP);
        g_effects->update(FIXED_TIMESTEP);

        //if player collided with enemy
        if (!g_current_scene->m_state.player->is_active() && g_current_scene != g_menu) {
            switch_to_scene(g_current_scene);
            lives_left -= 1;
        }

        //if (g_is_colliding_bottom == false && g_current_scene->m_state.player->m_collided_bottom) g_effects->start(SHAKE, 1.0f);

        g_is_colliding_bottom = g_current_scene->m_state.player->m_collided_bottom;

        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;

    /* -- VIEW MATRIX --  */
    g_view_matrix = glm::mat4(1.0f);
    float player_x = g_current_scene->m_state.player->get_position().x;
    float player_y = g_current_scene->m_state.player->get_position().y;

    /* PAUSE SCREEN */
    pause_menu->set_position(glm::vec3(player_x, -3.0f, 0.0f));
    pause_menu->update(delta_time, NULL, NULL, 0, NULL);
    pause_menu->scale(glm::vec3(6.0f, 3.0f, 0.0f));
    
    LOG("Player coordinates - X: " << player_x << ", Y: " << player_y);

    float view_matrix_x = -g_current_scene->m_state.player->get_position().x, 
          view_matrix_y = 3.75;
    if (g_current_scene == g_menu) {
        view_matrix_x = 0;
        view_matrix_y = 0;
    }
    if (g_current_scene->m_state.player->get_position().x > 19.1f) view_matrix_x = -19.1f;

    glm::vec3 view_matrix_postion = glm::vec3(view_matrix_x, view_matrix_y, 0.0f);
    g_view_matrix = glm::translate(g_view_matrix, view_matrix_postion);

    LOG("VM X-cord: " << view_matrix_x << ", Y-cord: " <<  view_matrix_y);

    /* -- LIVES -- */
    if(view_matrix_x != -19.1f)
        for (size_t i = 0; i < NUM_OF_LIVES; i++) lives[i].set_position(glm::vec3(player_x - 4.5 + i * 0.75, -0.5, 0));
    for (size_t i = 0; i < NUM_OF_LIVES; i++) lives[i].update(delta_time, NULL, NULL, 0, NULL);

    /* -- SCENE SWITCHING -- */
    if (g_current_scene->m_state.player->get_position().y < -10.0f ){
        switch_to_scene(g_current_scene);
        lives_left -= 1;
    }
    if (lives_left == 0) {
        g_game_won = false;
    }

    if (g_current_scene == g_levelA && g_current_scene->m_state.player->get_position().x > 25.0f) switch_to_scene(g_levelB);
    if (g_current_scene == g_levelB && g_current_scene->m_state.player->get_position().x > 25.0f) switch_to_scene(g_levelC);
    if (g_current_scene == g_levelC && g_current_scene->m_state.player->get_position().x > 19.5f) {
        g_game_won = true;
        g_current_scene->m_state.player->set_movement(glm::vec3(0.0f));
    }


}

void render()
{
    g_shader_program.SetViewMatrix(g_view_matrix);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(g_shader_program.programID);
    g_current_scene->render(&g_shader_program);
    g_effects->render();
    for (size_t i = 0; i < NUM_OF_LIVES; i++) {
        if (i + 1 > lives_left) lives[i].m_texture_id = Utility::load_texture("assets/sad_heart.png");
        if (g_current_scene != g_menu) lives[i].render(&g_shader_program);
    }
    if(g_paused)
        pause_menu->render(&g_shader_program);

    if(lives_left == 0 && g_game_won == false)
        Utility::draw_text(&g_shader_program, Utility::load_texture("assets/font1.png"), "You Lose", 0.75f, 0.0f, glm::vec3(g_current_scene->m_state.player->get_position().x - 2.5f, g_current_scene->m_state.player->get_position().y + 2.0f, 0.0f));
    else if(g_game_won == true)
        Utility::draw_text(&g_shader_program, Utility::load_texture("assets/font1.png"), "You Win", 0.75f, 0.0f, glm::vec3(g_current_scene->m_state.player->get_position().x - 2.5f, g_current_scene->m_state.player->get_position().y + 2.0f, 0.0f));


    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete [] lives;
    delete pause_menu;
    delete g_menu;
    delete g_levelA;
    delete g_levelB;
    delete g_levelC;
    delete g_effects;
}

// ––––– DRIVER GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();

        if (g_current_scene->m_state.next_scene_id >= 0) switch_to_scene(g_levels[g_current_scene->m_state.next_scene_id]);

        render();
    }

    shutdown();
    return 0;
}
