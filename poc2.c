/*
 * Chuva de Meteoros Matemáticos - Allegro 5
 *
 * Meteoros caem do céu com contas aleatórias.
 * Digite a resposta e aperte ENTER para destruir o meteoro.
 * Se um meteoro tocar o chão, você perde uma vida.
 *
 * Compilar (Linux/MSYS2):
 *   gcc meteoros.c -o meteoros $(pkg-config --cflags --libs allegro-5 allegro_font-5 allegro_primitives-5)
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define LARGURA       800
#define ALTURA        600
#define FPS           60
#define MAX_METEOROS  12
#define MAX_ESTRELAS  120
#define MAX_EXPLOSOES 12
#define CHAO          (ALTURA - 60)

typedef struct {
    float x, y, vel, raio;
    int ativo;
    int resposta;
    char conta[32];
} Meteoro;

typedef struct {
    float x, y, brilho;
} Estrela;

typedef struct {
    float x, y;
    int timer;
    int ativo;
    int acerto; /* 1 = destruído pelo jogador, 0 = bateu no chão */
} Explosao;

Meteoro meteoros[MAX_METEOROS];
Estrela estrelas[MAX_ESTRELAS];
Explosao explosoes[MAX_EXPLOSOES];

int pontos = 0;
int vidas = 5;
int game_over = 0;
char digitado[16] = "";

/* ---------- utilidades ---------- */

int aleatorio(int min, int max) {
    return min + rand() % (max - min + 1);
}

/* Desenha texto da fonte embutida em escala (ela é pequena, 8px) */
void texto(ALLEGRO_FONT *f, ALLEGRO_COLOR cor, float x, float y,
           float escala, int flags, const char *s) {
    ALLEGRO_TRANSFORM t, antigo;
    antigo = *al_get_current_transform();
    al_identity_transform(&t);
    al_scale_transform(&t, escala, escala);
    al_translate_transform(&t, x, y);
    al_use_transform(&t);
    al_draw_text(f, cor, 0, 0, flags, s);
    al_use_transform(&antigo);
}

/* ---------- contas ---------- */

void gerar_conta(Meteoro *m) {
    int a, b, op = aleatorio(0, 3);

    /* Dificuldade sobe com os pontos */
    int limite = 10 + pontos / 2;
    if (limite > 50) limite = 50;

    switch (op) {
        case 0: /* soma */
            a = aleatorio(1, limite);
            b = aleatorio(1, limite);
            m->resposta = a + b;
            sprintf(m->conta, "%d+%d", a, b);
            break;
        case 1: /* subtração (sem resultado negativo) */
            a = aleatorio(1, limite);
            b = aleatorio(1, limite);
            if (b > a) { int tmp = a; a = b; b = tmp; }
            m->resposta = a - b;
            sprintf(m->conta, "%d-%d", a, b);
            break;
        case 2: /* multiplicação */
            a = aleatorio(2, 10);
            b = aleatorio(2, 10);
            m->resposta = a * b;
            sprintf(m->conta, "%dx%d", a, b);
            break;
        default: /* divisão exata */
            b = aleatorio(2, 10);
            m->resposta = aleatorio(1, 10);
            a = b * m->resposta;
            sprintf(m->conta, "%d/%d", a, b);
            break;
    }
}

void criar_meteoro(void) {
    for (int i = 0; i < MAX_METEOROS; i++) {
        if (!meteoros[i].ativo) {
            Meteoro *m = &meteoros[i];
            m->ativo = 1;
            m->raio = aleatorio(32, 42);
            m->x = aleatorio((int)m->raio, LARGURA - (int)m->raio);
            m->y = -m->raio;
            m->vel = 0.6f + aleatorio(0, 60) / 100.0f + pontos * 0.03f;
            if (m->vel > 3.5f) m->vel = 3.5f;
            gerar_conta(m);
            return;
        }
    }
}

void criar_explosao(float x, float y, int acerto) {
    for (int i = 0; i < MAX_EXPLOSOES; i++) {
        if (!explosoes[i].ativo) {
            explosoes[i].ativo = 1;
            explosoes[i].x = x;
            explosoes[i].y = y;
            explosoes[i].timer = 30;
            explosoes[i].acerto = acerto;
            return;
        }
    }
}

/* ---------- lógica ---------- */

void reiniciar(void) {
    memset(meteoros, 0, sizeof(meteoros));
    memset(explosoes, 0, sizeof(explosoes));
    pontos = 0;
    vidas = 5;
    game_over = 0;
    digitado[0] = '\0';
}

void conferir_resposta(void) {
    if (digitado[0] == '\0') return;
    int valor = atoi(digitado);

    /* Destrói o meteoro mais baixo (mais perigoso) com essa resposta */
    int alvo = -1;
    for (int i = 0; i < MAX_METEOROS; i++) {
        if (meteoros[i].ativo && meteoros[i].resposta == valor) {
            if (alvo == -1 || meteoros[i].y > meteoros[alvo].y)
                alvo = i;
        }
    }

    if (alvo != -1) {
        meteoros[alvo].ativo = 0;
        criar_explosao(meteoros[alvo].x, meteoros[alvo].y, 1);
        pontos++;
    }
    digitado[0] = '\0';
}

void atualizar(int *contador_spawn) {
    /* estrelas piscando */
    for (int i = 0; i < MAX_ESTRELAS; i++) {
        estrelas[i].brilho += (aleatorio(-10, 10)) / 200.0f;
        if (estrelas[i].brilho < 0.3f) estrelas[i].brilho = 0.3f;
        if (estrelas[i].brilho > 1.0f) estrelas[i].brilho = 1.0f;
    }

    if (game_over) return;

    /* surgimento de meteoros: mais rápido conforme os pontos */
    int intervalo = 120 - pontos * 3;
    if (intervalo < 40) intervalo = 40;
    if (++(*contador_spawn) >= intervalo) {
        *contador_spawn = 0;
        criar_meteoro();
    }

    for (int i = 0; i < MAX_METEOROS; i++) {
        Meteoro *m = &meteoros[i];
        if (!m->ativo) continue;
        m->y += m->vel;
        if (m->y + m->raio >= CHAO) {
            m->ativo = 0;
            criar_explosao(m->x, CHAO, 0);
            vidas--;
            if (vidas <= 0) game_over = 1;
        }
    }

    for (int i = 0; i < MAX_EXPLOSOES; i++) {
        if (explosoes[i].ativo && --explosoes[i].timer <= 0)
            explosoes[i].ativo = 0;
    }
}

/* ---------- desenho ---------- */

void desenhar_meteoro(Meteoro *m, ALLEGRO_FONT *fonte) {
    /* rastro de fogo */
    for (int k = 6; k >= 1; k--) {
        float a = 0.12f * (7 - k);
        al_draw_filled_circle(m->x, m->y - k * m->raio * 0.35f,
                              m->raio * (1.0f - k * 0.1f),
                              al_map_rgba_f(1.0f * a, 0.45f * a, 0.0f, a));
    }

    /* corpo */
    al_draw_filled_circle(m->x, m->y, m->raio, al_map_rgb(110, 75, 50));
    al_draw_circle(m->x, m->y, m->raio, al_map_rgb(255, 140, 40), 3);

    /* crateras */
    al_draw_filled_circle(m->x - m->raio * 0.45f, m->y - m->raio * 0.40f,
                          m->raio * 0.15f, al_map_rgb(80, 55, 35));
    al_draw_filled_circle(m->x + m->raio * 0.50f, m->y + m->raio * 0.35f,
                          m->raio * 0.12f, al_map_rgb(80, 55, 35));

    /* conta */
    texto(fonte, al_map_rgb(255, 255, 255), m->x, m->y - 8, 2.0f,
          ALLEGRO_ALIGN_CENTRE, m->conta);
}

void desenhar(ALLEGRO_FONT *fonte) {
    al_clear_to_color(al_map_rgb(8, 8, 28));

    for (int i = 0; i < MAX_ESTRELAS; i++) {
        float b = estrelas[i].brilho;
        al_draw_filled_circle(estrelas[i].x, estrelas[i].y, 1.2f,
                              al_map_rgb_f(b, b, b));
    }

    /* chão */
    al_draw_filled_rectangle(0, CHAO, LARGURA, ALTURA, al_map_rgb(30, 70, 30));
    al_draw_line(0, CHAO, LARGURA, CHAO, al_map_rgb(60, 140, 60), 3);

    for (int i = 0; i < MAX_METEOROS; i++)
        if (meteoros[i].ativo) desenhar_meteoro(&meteoros[i], fonte);

    for (int i = 0; i < MAX_EXPLOSOES; i++) {
        Explosao *e = &explosoes[i];
        if (!e->ativo) continue;
        float p = (30 - e->timer) / 30.0f;
        float a = 1.0f - p;
        ALLEGRO_COLOR cor = e->acerto
            ? al_map_rgba_f(0.3f * a, 1.0f * a, 0.4f * a, a)
            : al_map_rgba_f(1.0f * a, 0.3f * a, 0.1f * a, a);
        al_draw_circle(e->x, e->y, 10 + p * 60, cor, 4);
        al_draw_filled_circle(e->x, e->y, 20 * a, cor);
    }

    /* HUD */
    char buf[64];
    sprintf(buf, "Pontos: %d", pontos);
    texto(fonte, al_map_rgb(255, 255, 255), 10, 10, 2.0f, 0, buf);
    sprintf(buf, "Vidas: %d", vidas);
    texto(fonte, al_map_rgb(255, 90, 90), LARGURA - 10, 10, 2.0f,
          ALLEGRO_ALIGN_RIGHT, buf);

    /* caixa de resposta */
    al_draw_filled_rounded_rectangle(LARGURA / 2 - 120, CHAO + 12,
                                     LARGURA / 2 + 120, CHAO + 48,
                                     8, 8, al_map_rgb(20, 20, 20));
    sprintf(buf, "> %s_", digitado);
    texto(fonte, al_map_rgb(255, 230, 80), LARGURA / 2, CHAO + 22, 2.5f,
          ALLEGRO_ALIGN_CENTRE, buf);

    if (game_over) {
        al_draw_filled_rectangle(0, 0, LARGURA, ALTURA, al_map_rgba(0, 0, 0, 170));
        texto(fonte, al_map_rgb(255, 80, 80), LARGURA / 2, ALTURA / 2 - 60,
              5.0f, ALLEGRO_ALIGN_CENTRE, "GAME OVER");
        sprintf(buf, "Pontos: %d", pontos);
        texto(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, ALTURA / 2 + 10,
              3.0f, ALLEGRO_ALIGN_CENTRE, buf);
        texto(fonte, al_map_rgb(200, 200, 200), LARGURA / 2, ALTURA / 2 + 60,
              2.0f, ALLEGRO_ALIGN_CENTRE, "R = reiniciar   ESC = sair");
    }

    al_flip_display();
}

/* ---------- main ---------- */

int main(void) {
    srand((unsigned)time(NULL));

    if (!al_init()) { fprintf(stderr, "Falha ao iniciar Allegro\n"); return 1; }
    al_install_keyboard();
    al_init_font_addon();
    al_init_primitives_addon();

    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 4, ALLEGRO_SUGGEST);

    ALLEGRO_DISPLAY *tela = al_create_display(LARGURA, ALTURA);
    if (!tela) { fprintf(stderr, "Falha ao criar a janela\n"); return 1; }
    al_set_window_title(tela, "Chuva de Meteoros Matematicos");

    ALLEGRO_FONT *fonte = al_create_builtin_font();
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE *fila = al_create_event_queue();

    al_register_event_source(fila, al_get_display_event_source(tela));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_timer_event_source(timer));

    for (int i = 0; i < MAX_ESTRELAS; i++) {
        estrelas[i].x = aleatorio(0, LARGURA);
        estrelas[i].y = aleatorio(0, CHAO);
        estrelas[i].brilho = aleatorio(30, 100) / 100.0f;
    }

    reiniciar();

    int rodando = 1, redesenhar = 1, contador_spawn = 0;
    al_start_timer(timer);

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        switch (ev.type) {
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                rodando = 0;
                break;

            case ALLEGRO_EVENT_TIMER:
                atualizar(&contador_spawn);
                redesenhar = 1;
                break;

            case ALLEGRO_EVENT_KEY_CHAR: {
                int k = ev.keyboard.keycode;
                int c = ev.keyboard.unichar;

                if (k == ALLEGRO_KEY_ESCAPE) { rodando = 0; break; }

                if (game_over) {
                    if (k == ALLEGRO_KEY_R) { reiniciar(); contador_spawn = 0; }
                    break;
                }

                if (k == ALLEGRO_KEY_ENTER || k == ALLEGRO_KEY_PAD_ENTER) {
                    conferir_resposta();
                } else if (k == ALLEGRO_KEY_BACKSPACE) {
                    size_t n = strlen(digitado);
                    if (n > 0) digitado[n - 1] = '\0';
                } else if (c >= '0' && c <= '9') {
                    size_t n = strlen(digitado);
                    if (n < 6) { digitado[n] = (char)c; digitado[n + 1] = '\0'; }
                }
                break;
            }
        }

        if (redesenhar && al_is_event_queue_empty(fila)) {
            redesenhar = 0;
            desenhar(fonte);
        }
    }

    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);
    al_destroy_display(tela);
    return 0;
}
