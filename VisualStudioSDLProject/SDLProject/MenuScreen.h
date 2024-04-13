#include "Scene.h"

class MenuScreen : public Scene {
public:

    ~MenuScreen();

    void initialise() override;
    void update(float delta_time) override;
    void render(ShaderProgram* program) override;
};
