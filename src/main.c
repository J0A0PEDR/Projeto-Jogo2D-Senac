/*
 * Defesa Matemática — ponto de partida.
 * Bibliotecas: Allegro 5 (core, primitives, font).
 * Este arquivo abre uma janela e exibe o título.
 */
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

int main(void) {
    if (!al_init()) {
        fprintf(stderr, "Falha ao iniciar Allegro 5.\n");
        return 1;
    }
    if (!al_install_keyboard()) {
        fprintf(stderr, "Falha ao iniciar teclado.\n");
        return 1;
    }
    if (!al_init_primitives_addon()) {
        fprintf(stderr, "Falha ao iniciar primitives.\n");
        return 1;
    }
    al_init_font_addon();

    ALLEGRO_DISPLAY *display = al_create_display(960, 540);
    if (!display) {
        fprintf(stderr, "Falha ao criar janela.\n");
        return 1;
    }
    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    if (!queue) {
        fprintf(stderr, "Falha ao criar fila de eventos.\n");
        al_destroy_display(display);
        return 1;
    }
    ALLEGRO_FONT *font = al_create_builtin_font();
    if (!font) {
        fprintf(stderr, "Falha ao criar fonte.\n");
        al_destroy_event_queue(queue);
        al_destroy_display(display);
        return 1;
    }

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());

    bool running = true;
    while (running) {
        al_clear_to_color(al_map_rgb(13, 22, 42));
        al_draw_filled_rectangle(0, 450, 960, 540, al_map_rgb(35, 78, 68));
        al_draw_text(font, al_map_rgb(255, 255, 255), 480, 180,
                     ALLEGRO_ALIGN_CENTER, "DEFESA MATEMATICA");
        al_draw_text(font, al_map_rgb(200, 220, 240), 480, 220,
                     ALLEGRO_ALIGN_CENTER, "Proteja a cidade resolvendo contas!");
        al_draw_text(font, al_map_rgb(200, 220, 240), 480, 260,
                     ALLEGRO_ALIGN_CENTER, "Pressione ESC para sair");
        al_flip_display();

        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE ||
            (event.type == ALLEGRO_EVENT_KEY_DOWN &&
             event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
            running = false;
        }
    }

    al_destroy_font(font);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    return 0;
}
