#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include <vector>

#include "lodepng.h"

class PngImageAsset {
    public:

        PngImageAsset(const char *path, float alpha = 1.0) {
            ASSERT_EXIT(lodepng::decode(m_buffer, m_width, m_height, path));

            // Scale transparency bytes by normalised alpha value
            for (unsigned int i = 3; i < m_buffer.size(); i += 4) {
                m_buffer[i] = static_cast<unsigned char>(m_buffer[i] * alpha);
            }
        }

        uint8_t *getBuffer(void) {
            return &m_buffer[0];
        }

        uint32_t getBufferSize(void) {
            return m_buffer.size();
        }

        uint32_t getWidth(void) {
            return m_width;
        }

        uint32_t getHeight(void) {
            return m_height;
        }

    private:
        std::vector<unsigned char> m_buffer;
        uint32_t m_width;
        uint32_t m_height;
};

void setImageColour(PngImageAsset *img, tsl::Color colour) {
    auto buffer = img->getBuffer();
    uint8_t r = colour.r << 4;
    uint8_t g = colour.g << 4;
    uint8_t b = colour.b << 4;
    for (unsigned int i = 0; i < img->getBufferSize(); i += 4) {
        buffer[i] = static_cast<unsigned char>(buffer[i] * r);
        buffer[i+1] = static_cast<unsigned char>(buffer[i+1] * g);
        buffer[i+2] = static_cast<unsigned char>(buffer[i+2] * b);
    }
}

class InvisibleOverlayFrame : public tsl::elm::OverlayFrame {

    public:

        InvisibleOverlayFrame(const std::string& title, const std::string& subtitle) : tsl::elm::OverlayFrame(title, subtitle) {}

        virtual void draw(tsl::gfx::Renderer *renderer) override {
            renderer->fillScreen(a({0x0, 0x0, 0x0, 0x0}));

            renderer->drawString(this->m_title.c_str(), false, 20, 50, 30, a(tsl::style::color::ColorText));
            renderer->drawString(this->m_subtitle.c_str(), false, 20, 70, 15, a(tsl::style::color::ColorDescription));

            if (this->m_contentElement != nullptr)
                this->m_contentElement->frame(renderer);
        }

};

class UnboundedDrawerer : public tsl::elm::Element {

    public:

        UnboundedDrawerer(std::function<void(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h)> renderFunc) : tsl::elm::Element(), m_renderFunc(renderFunc) {}
        virtual ~UnboundedDrawerer() {}

        virtual void draw(tsl::gfx::Renderer* renderer) override {
            this->m_renderFunc(renderer, ELEMENT_BOUNDS(this));
        }

        virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {

        }

    private:
        std::function<void(tsl::gfx::Renderer*, s32 x, s32 y, s32 w, s32 h)> m_renderFunc;
};

class ControllerOverlayGui : public tsl::Gui {

    public:

        ControllerOverlayGui(uint8_t alpha = 0xff)
         : m_xpos(5)
         , m_ypos(5)
         , m_colour_active(0x0, 0xF, 0xD, alpha >> 4)
         , m_colour_inactive(0xA, 0xA, 0xA, alpha >> 4)
         , m_buttons{}
         , m_lstick{}
         , m_rstick{} 
         , m_img_base("sdmc:/img/switchpro-base.png", float(alpha) / 0xff)
         , m_img_dpad("sdmc:/img/switchpro-dpad.png", float(alpha) / 0xff)
         , m_img_lbutton("sdmc:/img/switchpro-lbutton.png", float(alpha) / 0xff)
         , m_img_rbutton("sdmc:/img/switchpro-rbutton.png", float(alpha) / 0xff) {

            tsl::Color c = {0xA, 0xA, 0xA, alpha >> 4};
            setImageColour(&m_img_dpad, c);
            setImageColour(&m_img_lbutton, c);
            setImageColour(&m_img_rbutton, c);
        }

        virtual tsl::elm::Element* createUI() override { 
            auto frame = new InvisibleOverlayFrame("", "");

            auto buttons = new UnboundedDrawerer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                renderer->drawBitmap(m_xpos,       m_ypos, m_img_base.getWidth(), m_img_base.getHeight(), m_img_base.getBuffer(), tsl::gfx::PixelBlendMode_Dst);
                renderer->drawBitmap(m_xpos + 69,  m_ypos + 71, m_img_dpad.getWidth(), m_img_dpad.getHeight(), m_img_dpad.getBuffer(), tsl::gfx::PixelBlendMode_Dst);
                //renderer->drawBitmap(m_xpos + 28,  m_ypos + 1,  m_img_lbutton.getWidth(), m_img_lbutton.getHeight(), m_img_lbutton.getBuffer());
                //renderer->drawBitmap(m_xpos + 166, m_ypos + 1,  m_img_rbutton.getWidth(), m_img_rbutton.getHeight(), m_img_rbutton.getBuffer());

                renderer->drawCircle(m_xpos + 215, m_ypos + 54, 9, true, m_buttons & HidNpadButton_A ? m_colour_active : m_colour_inactive); // A button 
                renderer->drawCircle(m_xpos + 195, m_ypos + 71, 9, true, m_buttons & HidNpadButton_B ? m_colour_active : m_colour_inactive); // B button
                renderer->drawCircle(m_xpos + 195, m_ypos + 37, 9, true, m_buttons & HidNpadButton_X ? m_colour_active : m_colour_inactive); // X button
                renderer->drawCircle(m_xpos + 175, m_ypos + 54, 9, true, m_buttons & HidNpadButton_Y ? m_colour_active : m_colour_inactive); // Y button

                renderer->drawCircle(m_xpos + 96,  m_ypos + 35, 5, true, m_buttons & HidNpadButton_Minus ? m_colour_active : m_colour_inactive); // - button
                renderer->drawCircle(m_xpos + 159, m_ypos + 35, 5, true, m_buttons & HidNpadButton_Plus  ? m_colour_active : m_colour_inactive); // + button

                renderer->drawCircle(m_xpos + 57  + (9 * m_lstick.x / JOYSTICK_MAX), m_ypos + 54 + (9 * -m_lstick.y / JOYSTICK_MAX), 14, true, (m_lstick.x != 0 || m_lstick.y != 0 || (m_buttons & HidNpadButton_StickL)) ? m_colour_active : m_colour_inactive); // Left stick
                renderer->drawCircle(m_xpos + 161 + (9 * m_rstick.x / JOYSTICK_MAX), m_ypos + 89 + (9 * -m_rstick.y / JOYSTICK_MAX), 14, true, (m_rstick.x != 0 || m_rstick.y != 0 || (m_buttons & HidNpadButton_StickR)) ? m_colour_active : m_colour_inactive); // Right stick
            });
            frame->setContent(buttons);

            return frame;
        }


        virtual void update() override {

        }

        virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
            m_buttons = keysDown | keysHeld;
            m_lstick = joyStickPosLeft;
            m_rstick = joyStickPosRight;

            if ((keysHeld & HidNpadButton_StickL) && (keysHeld & HidNpadButton_StickR)) {
                tsl::goBack();
            }

            return true;
        }

    private:
        s32 m_xpos; 
        s32 m_ypos;
        tsl::Color m_colour_active;
        tsl::Color m_colour_inactive;

        u64 m_buttons;
        HidAnalogStickState m_lstick;
        HidAnalogStickState m_rstick;

        PngImageAsset m_img_base;
        PngImageAsset m_img_dpad;
        PngImageAsset m_img_lbutton;
        PngImageAsset m_img_rbutton;
};

class MainGui : public tsl::Gui {

    public:
        MainGui() { }

        virtual tsl::elm::Element* createUI() override {
            auto frame = new tsl::elm::OverlayFrame("Controller Overlay", "v1.0.0");

            auto list = new tsl::elm::List();

            auto overlay_gui = new tsl::elm::ListItem("Controller Overlay");
            overlay_gui->setClickListener([](std::uint64_t keys) {
                if (keys & HidNpadButton_A) {
                    tsl::hlp::requestForeground(false);
                    tsl::changeTo<ControllerOverlayGui>(0xc0);
                    return true;
                }

                return false;
            });
            list->addItem(overlay_gui);
            
            frame->setContent(list);
            
            return frame;
        }

        virtual void update() override {

        }

        virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
            return false;
        }
};

class Overlay : public tsl::Overlay {

    public:

        virtual void initServices() override {
            //ASSERT_EXIT(romfsInit());
            ASSERT_EXIT(fsdevMountSdmc());

            //hidInitializeNpad();
        }

        virtual void exitServices() override {
            //romfsExit();
            fsdevUnmountAll();
        }

        virtual void onShow() override {}
        virtual void onHide() override {}

        virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
            return initially<MainGui>();

            //return initially<ControllerTestGui>();
            //return initially<ControllerOverlayGui>();
        }
};

int main(int argc, char **argv) {
    return tsl::loop<Overlay>(argc, argv);
}
