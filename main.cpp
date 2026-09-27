#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

int main() {
    // 1. Inicializa o Allegro e os seus módulos de desenho e texto
    al_init();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();

    // 2. Cria a janela do jogo (Largura: 400px, Altura: 600px)
    ALLEGRO_DISPLAY* display = al_create_display(400, 600);
    al_set_window_title(display, "Jogo de Matemática - Defesa da Cidade");

    // 3. Cria a fonte padrão para escrever os textos na tela
    ALLEGRO_FONT* font = al_create_builtin_font();

    // 4. Cria a fila para detectar eventos (como fechar a janela no X)
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    al_register_event_source(queue, al_get_display_event_source(display));

    bool jogando = true;

    // Loop Principal (Onde a tela é desenhada continuamente)
    while (jogando) {
        ALLEGRO_EVENT event;
        // Verifica se o usuário clicou no 'X' para fechar a janela
        if (al_get_next_event(queue, &event)) {
            if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                jogando = false;
            }
        }

        // ==========================================
        // INTERFACE DA TELA
        // ==========================================

        // Fundo: Azul escuro (Céu/Espaço)
        al_clear_to_color(al_map_rgb(15, 15, 35));

        // --- A. DESENHO DOS METEOROS E EQUAÇÕES ---
        // Meteoro 1 (Superior esquerdo)
        al_draw_filled_circle(100, 150, 25, al_map_rgb(160, 60, 30));  // Rocha
        al_draw_circle(100, 150, 28, al_map_rgb(255, 120, 0), 3);       // Fogo em volta
        al_draw_text(font, al_map_rgb(255, 255, 255), 100, 145, ALLEGRO_ALIGN_CENTER, "8 + 5 =");

        // Meteoro 2 (Centro)
        al_draw_filled_circle(240, 210, 30, al_map_rgb(160, 60, 30));
        al_draw_circle(240, 210, 33, al_map_rgb(255, 120, 0), 3);
        al_draw_text(font, al_map_rgb(255, 255, 255), 240, 205, ALLEGRO_ALIGN_CENTER, "12 - 7 =");

        // Meteoro 3 (Direita)
        al_draw_filled_circle(330, 270, 25, al_map_rgb(160, 60, 30));
        al_draw_circle(330, 270, 28, al_map_rgb(255, 120, 0), 3);
        al_draw_text(font, al_map_rgb(255, 255, 255), 330, 265, ALLEGRO_ALIGN_CENTER, "6 x 4 =");


        // --- B. CAMPO DE FORÇA DA CIDADE ---
        // Arco azul simulando o domo protetor
        al_draw_arc(200, 520, 180, 3.14159, 3.14159, al_map_rgb(80, 180, 255), 3);


        // --- C. SILHUETA DOS PRÉDIOS DA CIDADE ---
        ALLEGRO_COLOR cor_predio = al_map_rgb(40, 50, 75);
        al_draw_filled_rectangle(30, 420, 80, 520, cor_predio);
        al_draw_filled_rectangle(90, 380, 140, 520, cor_predio);
        al_draw_filled_rectangle(150, 330, 200, 520, cor_predio);
        al_draw_filled_rectangle(210, 390, 260, 520, cor_predio);
        al_draw_filled_rectangle(270, 370, 320, 520, cor_predio);
        al_draw_filled_rectangle(330, 410, 370, 520, cor_predio);


        // --- D. INTERFACE INFERIOR (HUD) ---
        // Fundo do painel inferior
        al_draw_filled_rectangle(0, 520, 400, 600, al_map_rgb(10, 10, 20));

        // Texto: VIDA DA CIDADE
        al_draw_text(font, al_map_rgb(255, 255, 255), 20, 530, ALLEGRO_ALIGN_LEFT, "VIDA DA CIDADE");

        // Texto: LEVEL 3
        al_draw_text(font, al_map_rgb(255, 215, 0), 380, 560, ALLEGRO_ALIGN_RIGHT, "LEVEL 3");

        // Desenhar os blocos da Barra de Vida
        int x_inicial = 20;
        int y_barra = 555;
        int largura_bloco = 20;
        int altura_bloco = 25;

        for (int i = 0; i < 12; i++) {
            ALLEGRO_COLOR cor_bloco;

            // Os 3 primeiros blocos são vermelhos (vida gasta/dano)
            if (i < 3) {
                cor_bloco = al_map_rgb(220, 40, 40);
            } else { // O restante é verde (vida atual)
                cor_bloco = al_map_rgb(40, 220, 40);
            }

            // Desenha o bloco preenchido e a borda preta
            al_draw_filled_rectangle(x_inicial + (i * 22), y_barra, x_inicial + (i * 22) + largura_bloco, y_barra + altura_bloco, cor_bloco);
            al_draw_rectangle(x_inicial + (i * 22), y_barra, x_inicial + (i * 22) + largura_bloco, y_barra + altura_bloco, al_map_rgb(0, 0, 0), 1);
        }

        // Atualiza a tela para exibir tudo o que foi desenhado
        al_flip_display();
        al_rest(0.016); // Pausa de aproximadamente ~60 FPS
    }

    // 5. Liberação de memória ao fechar o programa
    al_destroy_font(font);
    al_destroy_event_queue(queue);
    al_destroy_display(display);

    return 0;
}
