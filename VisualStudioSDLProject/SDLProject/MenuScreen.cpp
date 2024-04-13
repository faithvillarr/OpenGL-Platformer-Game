#include "MenuScreen.h"
#include "Utility.h"

#define LEVEL_WIDTH 1
#define LEVEL_HEIGHT 1

unsigned int MENU_DATA[] = { 0 };
Entity* title_card;

MenuScreen::~MenuScreen()
{
    /* destroy entities here */
    //delete[] m_state.enemies;
    //delete    m_state.player;
    delete    m_state.player;
    delete    m_state.map;
    Mix_FreeMusic(m_state.bgm);
}

void MenuScreen::initialise() {
    m_state.next_scene_id = -1;

    GLuint map_texture_id = Utility::load_texture("assets/title-tiles.png");
    m_state.map = new Map(LEVEL_WIDTH, LEVEL_HEIGHT, MENU_DATA, map_texture_id, 1.0f, 1, 1);
    
    /* Entities */
    title_card = new Entity();
    GLuint title_texture_id = Utility::load_texture("assets/title-card.png");
    title_card->m_texture_id = title_texture_id;
    title_card->set_position(glm::vec3(1.0f));
    title_card->set_movement(glm::vec3(0.0f));
    title_card->set_acceleration(glm::vec3(0.0f));
    title_card->scale(glm::vec3(8.0f, 4.0f, 1.0f));
    title_card->set_entity_type(LIVES);

    /* NULL PLAYER */
    // Existing 
    m_state.player = new Entity();
    m_state.player->set_entity_type(PLAYER);
    m_state.player->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    m_state.player->set_movement(glm::vec3(0.0f));
    m_state.player->set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));
    m_state.player->deactivate();


    /* BGM and SFX */
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

    m_state.bgm = Mix_LoadMUS("assets/Lady Brown.mp3");
    Mix_PlayMusic(m_state.bgm, -1);
    Mix_VolumeMusic(10.0f);
}

void MenuScreen::update(float delta_time) {
    title_card->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    title_card->update(delta_time, NULL, NULL, 0, m_state.map);
    title_card->scale(glm::vec3(10.0f, 8.0f, 1.0f));
}


void MenuScreen::render(ShaderProgram* program)
{
    title_card->render(program);
    m_state.map->render(program);

}