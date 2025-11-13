#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WIDTH  1024
#define MAX_HEIGHT 768

/* Protótipos */
void print_usage(const char *prog_name);
int  read_manual_input(int *width, int *height, int **pixels);
int  load_pbm_file(const char *filename, int *width, int *height, int **pixels);
void print_image(int *pixels, int width, int height);
void encode_region(int *pixels, int img_width,
                   int row_off, int col_off,
                   int w, int h,
                   char *code_buf, int *pos);

/* ------------------------------------------------------------------------- */

void print_usage(const char *prog_name) {
    printf("Uso: %s [-? | -m | -f ARQ]\n", prog_name);
    printf("Codifica imagens binarias dadas em arquivos PBM ou por dados\n");
    printf("informados manualmente.\n\n");
    printf("Argumentos:\n");
    printf(" -?, --help        : apresenta esta orientacao na tela.\n");
    printf(" -m, --manual      : modo de entrada manual (usuario informa a imagem).\n");
    printf(" -f, --file  ARQ   : considera a imagem representada no arquivo PBM.\n");
}

/* Entrada manual: usuario digita largura, altura e todos os pixels (0/1) */
int read_manual_input(int *width, int *height, int **pixels) {
    int w, h;

    printf("Modo manual selecionado.\n");
    printf("Informe largura (max %d): ", MAX_WIDTH);
    if (scanf("%d", &w) != 1) {
        fprintf(stderr, "Erro na leitura da largura.\n");
        return 1;
    }

    printf("Informe altura (max %d): ", MAX_HEIGHT);
    if (scanf("%d", &h) != 1) {
        fprintf(stderr, "Erro na leitura da altura.\n");
        return 1;
    }

    if (w <= 0 || w > MAX_WIDTH || h <= 0 || h > MAX_HEIGHT) {
        fprintf(stderr,
                "Error: Image dimensions exceed maximum allowed size of %dx%d.\n",
                MAX_WIDTH, MAX_HEIGHT);
        return 1;
    }

    int total = w * h;
    int *data = malloc(sizeof(int) * total);
    if (!data) {
        fprintf(stderr, "Erro de alocacao de memoria.\n");
        return 1;
    }

    printf("Informe os pixels (0 = branco, 1 = preto), separados por espaco ou enter:\n");
    for (int i = 0; i < total; i++) {
        int v;
        if (scanf("%d", &v) != 1 || (v != 0 && v != 1)) {
            fprintf(stderr, "Valor de pixel invalido.\n");
            free(data);
            return 1;
        }
        data[i] = v;
    }

    *width  = w;
    *height = h;
    *pixels = data;
    return 0;
}

/* Auxiliar: consome comentarios e espacos em branco apos o magic number */
static void skip_pbm_comments(FILE *fp) {
    int c;

    /* Pula espacos em branco e linhas de comentario que comecam com '#' */
    while ((c = fgetc(fp)) != EOF) {
        if (c == '#') {
            /* descarta ate o fim da linha */
            while ((c = fgetc(fp)) != EOF && c != '\n')
                ;
        } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            /* continua pulando whitespace */
            continue;
        } else {
            /* caractere util: devolve para o fluxo */
            ungetc(c, fp);
            break;
        }
    }
}

/* Le um arquivo PBM (formato texto, magic P1) */
int load_pbm_file(const char *filename, int *width, int *height, int **pixels) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir arquivo '%s'.\n", filename);
        return 1;
    }

    char magic[3];
    if (fscanf(fp, "%2s", magic) != 1 || strcmp(magic, "P1") != 0) {
        fprintf(stderr, "Formato PBM invalido (esperado 'P1').\n");
        fclose(fp);
        return 1;
    }

    /* pula comentarios e espacos antes das dimensoes */
    skip_pbm_comments(fp);

    int w, h;
    if (fscanf(fp, "%d %d", &w, &h) != 2) {
        fprintf(stderr, "Erro ao ler as dimensoes da imagem.\n");
        fclose(fp);
        return 1;
    }

    if (w <= 0 || w > MAX_WIDTH || h <= 0 || h > MAX_HEIGHT) {
        fprintf(stderr,
                "Error: Image dimensions exceed maximum allowed size of %dx%d.\n",
                MAX_WIDTH, MAX_HEIGHT);
        fclose(fp);
        return 1;
    }

    int total = w * h;
    int *data = malloc(sizeof(int) * total);
    if (!data) {
        fprintf(stderr, "Erro de alocacao de memoria.\n");
        fclose(fp);
        return 1;
    }

    int pixels_read = 0;
    while (pixels_read < total && !feof(fp)) {
        int v;
        if (fscanf(fp, "%d", &v) == 1) {
            if (v != 0 && v != 1) {
                fprintf(stderr, "Pixel invalido: %d.\n", v);
                free(data);
                fclose(fp);
                return 1;
            }
            data[pixels_read++] = v;
        } else {
            /* pode ter comentarios no meio? para este projeto,
               vamos assumir que nao. Se fscanf falhar, paramos. */
            break;
        }
    }

    if (pixels_read < total) {
        fprintf(stderr,
                "Arquivo PBM incompleto (%d/%d pixels lidos).\n",
                pixels_read, total);
        free(data);
        fclose(fp);
        return 1;
    }

    fclose(fp);
    *width  = w;
    *height = h;
    *pixels = data;
    return 0;
}

void print_image(int *pixels, int width, int height) {
    printf("Imagem lida (%dx%d):\n", width, height);
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            printf("%d ", pixels[r * width + c]);
        }
        printf("\n");
    }
}

/* Codificacao recursiva em P/B/X conforme o enunciado */
void encode_region(int *pixels, int img_width,
                   int row_off, int col_off,
                   int w, int h,
                   char *code_buf, int *pos) {
    if (w <= 0 || h <= 0) {
        return;
    }

    int first = pixels[row_off * img_width + col_off];
    int uniform = 1;

    for (int i = 0; i < h && uniform; i++) {
        for (int j = 0; j < w; j++) {
            if (pixels[(row_off + i) * img_width + (col_off + j)] != first) {
                uniform = 0;
                break;
            }
        }
    }

    if (uniform) {
        code_buf[(*pos)++] = (first == 1) ? 'P' : 'B';
        return;
    }

    /* nao uniforme: gera X e divide em quadrantes */
    code_buf[(*pos)++] = 'X';

    int mid_h = (h + 1) / 2;  /* parte de cima fica com a linha extra */
    int mid_w = (w + 1) / 2;  /* parte da esquerda fica com a coluna extra */

    /* 1o quadrante: superior esquerdo */
    encode_region(pixels, img_width, row_off, col_off,
                  mid_w, mid_h, code_buf, pos);

    /* 2o quadrante: superior direito */
    encode_region(pixels, img_width, row_off, col_off + mid_w,
                  w - mid_w, mid_h, code_buf, pos);

    /* 3o quadrante: inferior esquerdo */
    encode_region(pixels, img_width, row_off + mid_h, col_off,
                  mid_w, h - mid_h, code_buf, pos);

    /* 4o quadrante: inferior direito */
    encode_region(pixels, img_width, row_off + mid_h, col_off + mid_w,
                  w - mid_w, h - mid_h, code_buf, pos);
}

/* ------------------------------------------------------------------------- */

int main(int argc, char *argv[]) {
    int  width = 0, height = 0;
    int *pixels = NULL;

    if (argc == 1) {
        /* nenhum argumento -> mostra help */
        print_usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    } else if (strcmp(argv[1], "-m") == 0 || strcmp(argv[1], "--manual") == 0) {
        if (read_manual_input(&width, &height, &pixels) != 0) {
            return 1;
        }
    } else if (strcmp(argv[1], "-f") == 0 || strcmp(argv[1], "--file") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Erro: faltou informar o nome do arquivo PBM.\n");
            print_usage(argv[0]);
            return 1;
        }
        if (load_pbm_file(argv[2], &width, &height, &pixels) != 0) {
            return 1;
        }
    } else {
        fprintf(stderr, "Erro: argumento invalido.\n");
        print_usage(argv[0]);
        return 1;
    }

    print_image(pixels, width, height);

    int buf_size = width * height * 3 + 16;
    char *code_buf = malloc(buf_size);
    if (!code_buf) {
        fprintf(stderr, "Erro de alocacao do buffer de codigo.\n");
        free(pixels);
        return 1;
    }

    int pos = 0;
    encode_region(pixels, width, 0, 0, width, height, code_buf, &pos);
    code_buf[pos] = '\0';

    printf("Codigo resultante:\n%s\n", code_buf);

    free(pixels);
    free(code_buf);
    return 0;
}
