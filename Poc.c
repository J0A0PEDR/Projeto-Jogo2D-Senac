#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

// Configurações da Janela
const int LARGURA_TELA = 800;
const int ALTURA_TELA = 600;
const float FPS = 60.0;

// Estrutura para representar cada Meteoro
struct Meteoro {
    float x, y;
    float velocidade;
    float raio;
    std::string expressao;
};

// Função auxiliar para gerar contas aleatórias
std::string gerarContaAleatoria() {
    int num1 = rand() % 15 + 1;
    int num2 = rand() % 15 + 1;
    char operadores[] = { '+', '-', '*' };
    char op = operadores[rand() % 3];

    return std::to_string(num1) + " " + op + " " + std::to_string(num2);
}

int main() {
    srand(static_cast<unsigned int>(time(NULL)));

    // 1. Inicialização do Allegro e Addons
    if (!al_init()) {
        std::cerr << "Falha ao inicializar o Allegro!\n";
        return -1;
    }

    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_install_keyboard();

    // 2. Criação dos componentes centrais
    ALLEGRO_DISPLAY* display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();

    // Carrega a fonte padrão integrada do Allegro
    ALLEGRO_FONT* font = al_create_builtin_font();

    if (!display || !timer || !event_queue || !font) {
        std::cerr << "Falha ao criar os recursos do Allegro!\n";
        return -1;
    }

    // 3. Registro das fontes de eventos
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());

    std::vector<Meteoro> meteoros;
    int contadorFrames = 0;
    bool rodando = true;
    bool desenhar = true;

    al_start_timer(timer);

    // 4. Game Loop Principal
    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(event_queue, &ev);

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            desenhar = true;
            contadorFrames++;

            // Gera um novo meteoro a cada 1.5 segundos (~90 frames)
            if (contadorFrames % 90 == 0) {
                Meteoro m;
                m.raio = 35.0f;
                m.x = rand() % (LARGURA_TELA - static_cast<int>(m.raio * 2)) + m.raio;
                m.y = -m.raio; // Começa um pouco acima do topo da tela
                m.velocidade = 1.5f + static_cast<float>(rand() % 200) / 100.0f; // Velocidade aleatória entre 1.5 e 3.5
                m.expressao = gerarContaAleatoria();

                meteoros.push_back(m);
            }

            // Atualiza a posição dos meteoros existentes
            for (size_t i = 0; i < meteoros.size(); i++) {
                meteoros[i].y += meteoros[i].velocidade;

                // Remove meteoros que passaram da parte inferior da tela
                if (meteoros[i].y - meteoros[i].raio > ALTURA_TELA) {
                    meteoros.erase(meteoros.begin() + i);
                    i--;
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                rodando = false;
            }
        }

        // 5. Renderização / Desenho
        if (desenhar && al_is_event_queue_empty(event_queue)) {
            desenhar = false;

            // Limpa a tela com fundo escuro (espaço)
            al_clear_to_color(al_map_rgb(15, 15, 30));

            // Desenha cada meteoro e sua conta matemática
            for (const auto& m : meteoros) {
                // Corpo do meteoro (círculo marrom escuro)
                al_draw_filled_circle(m.x, m.y, m.raio, al_map_rgb(110, 60, 30));
                
                // Borda do meteoro (laranja)
                al_draw_circle(m.x, m.y, m.raio, al_map_rgb(230, 120, 20), 3.0f);

                // Texto com a conta matemática no centro do meteoro
                al_draw_text(
                    font,
                    al_map_rgb(255, 255, 255),
                    m.x,
                    m.y - 4, // Ajuste vertical do texto
                    ALLEGRO_ALIGN_CENTER,
                    m.expressao.c_str()
                );
            }

            al_flip_display();
        }
    }

    // 6. Finalização e Liberação de Memória
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);

    return 0;
}
