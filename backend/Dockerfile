# 1. Usa uma imagem limpa do Linux Ubuntu para compilar o código
FROM ubuntu:22.04 AS builder

# 2. Instala o compilador GCC básico
RUN apt-get update && apt-get install -y gcc make libc-dev

WORKDIR /app

# 3. Copia os arquivos do backend para o ambiente de compilação
COPY main.c mongoose.c mongoose.h ./

# 4. Compila o binário do Linux (idêntico ao comando do terminal)
RUN gcc main.c mongoose.c -o servidor_mongoose -lpthread

# 5. Cria a imagem final ultra leve apenas para rodar o sistema
FROM ubuntu:22.04
WORKDIR /app
COPY --from=builder /app/servidor_mongoose .

# 6. Comando que a nuvem vai executar para ligar o motor em C
CMD ["./servidor_mongoose"]
