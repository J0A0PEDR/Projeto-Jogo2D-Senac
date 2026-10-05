#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#define LARGURA 800
#define ALTURA 600
#define MAX_METEOROS 5

typedef struct {
    float x, y;
    float velocidade;
    int num1, num2;
    char operacao; // '+' ou '-'
    int resultado;
    bool ativo;
} Meteoro;

// Funcao para criar/reiniciar um meteoro
void criar_meteoro(Meteoro *m) {
    m->x = 50 + rand() % (LARGURA - 100);
    m->y = -50 - (rand() % 100);
    m->velocidade = 0.5f + ((float)rand() / RAND_MAX) * 1.0f;
    
    m->num1 = 1 + rand() % 20;
    m->num2 = 1 + rand() % 20;
    
    if (rand() % 2 == 0) {
        m->operacao = '+';
        m->resultado = m->num1 + m->num2;
    } else {
        m->operacao = '-';
        // Garante resultado positivo para facilitar a digitacao
        if (m->num1 < m->num2) {
            int temp = m->num1;
            m->num1 = m->num2;
            m->num2 = temp;
        }
        m->resultado = m->num1 - m->num2;
    }
    m->ativo = true;
}

int main() {
    srand((unsigned int)time(NULL));

    // Inicializacoes do Allegro
    if (!al_init()) return -1;
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_install_keyboard();

    ALLEGRO_DISPLAY *display = al_create_display(LARGURA, ALTURA);
    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_FONT *fonte = al_create_builtin_font(); // Fonte embutida padrao

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());

    Meteoro meteoros[MAX_METEOROS];
    for (int i = 0; i < MAX_METEOROS; i++) {
        criar_meteoro(&meteoros[i]);
    }

    char entrada_texto[10] = "";
    int indice_texto = 0;
    int pontos = 0;
    int vidas = 3;
    bool rodando = true;

    al_start_timer(timer);

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            if (vidas > 0) {
                // Atualiza posicao dos meteoros
                for (int i = 0; i < MAX_METEOROS; i++) {
                    if (meteoros[i].ativo) {
                        meteoros[i].y += meteoros[i].velocidade;
                        // Se atingir o chao
                        if (meteoros[i].y > ALTURA - 30) {
                            vidas--;
                            criar_meteoro(&meteoros[i]);
                        }
                    }
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {
            if (vidas > 0) {
                // Digitar numeros (0-9)
                if (ev.unichar >= '0' && ev.unichar <= '9') {
                    if (indice_texto < (int)sizeof(entrada_texto) - 1) {
                        entrada_texto[indice_texto++] = (char)ev.unichar;
                        entrada_texto[indice_texto] = '\0';
                    }
                }
                // Backspace para apagar
                else if (ev.keycode == ALLEGRO_KEY_BACKSPACE && indice_texto > 0) {
                    entrada_texto[--indice_texto] = '\0';
                }
                // Enter para disparar a resposta
                else if (ev.keycode == ALLEGRO_KEY_ENTER || ev.keycode == ALLEGRO_KEY_PAD_ENTER) {
                    if (indice_texto > 0) {
                        int resposta = atoi(entrada_texto);
                        bool acertou = false;

                        for (int i = 0; i < MAX_METEOROS; i++) {
                            if (meteoros[i].ativo && meteoros[i].resultado == resposta) {
                                pontos += 10;
                                criar_meteoro(&meteoros[i]);
                                acertou = true;
                                break; // Destroi um meteoro por vez
                            }
                        }

                        // Limpa o campo de texto apos enviar
                        indice_texto = 0;
                        entrada_texto[0] = '\0';
                    }
                }
            }
        }

        // Desenho na tela
        al_clear_to_color(al_map_rgb(10, 10, 30)); // Fundo azul escuro (espaco)

        if (vidas > 0) {
            // Desenha os meteoros
            for (int i = 0; i < MAX_METEOROS; i++) {
                if (meteoros[i].ativo) {
                    // Corpo do meteoro
                    al_draw_filled_circle(meteoros[i].x, meteoros[i].y, 35, al_map_rgb(180, 80, 20));
                    al_draw_circle(meteoros[i].x, meteoros[i].y, 35, al_map_rgb(255, 140, 0), 3);

                    // Texto da conta no meteoro
                    char conta_str[20];
                    sprintf(conta_str, "%d %c %d", meteoros[i].num1, meteoros[i].operacao, meteoros[i].num2);
                    al_draw_text(fonte, al_map_rgb(255, 255, 255), meteoros[i].x, meteoros[i].y - 5, ALLEGRO_ALIGN_CENTER, conta_str);
                }
            }

            // Interface do Jogador (HUD)
            al_draw_filled_rectangle(0, ALTURA - 40, LARGURA, ALTURA, al_map_rgb(40, 40, 50));
            
            char info_str[50];
            sprintf(info_str, "Pontos: %d  |  Vidas: %d", pontos, vidas);
            al_draw_text(fonte, al_map_rgb(255, 255, 255), 20, ALTURA - 25, 0, info_str);

            char entrada_str[30];
            sprintf(entrada_str, "Sua resposta: %s_", entrada_texto);
            al_draw_text(fonte, al_map_rgb(0, 255, 0), LARGURA - 200, ALTURA - 25, 0, entrada_str);
        } else {
            // Tela de Game Over
            al_draw_text(fonte, al_map_rgb(255, 50, 50), LARGURA / 2, ALTURA / 2 - 20, ALLEGRO_ALIGN_CENTER, "GAME OVER!");
            
            char final_str[50];
            sprintf(final_str, "Pontuacao final: %d", pontos);
            al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, ALTURA / 2 + 10, ALLEGRO_ALIGN_CENTER, final_str);
        }

        al_flip_display();
    }

    // Limpeza de recursos
    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);

    return 0;
}
