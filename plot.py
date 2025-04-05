import os
import re
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from matplotlib.backends.backend_pdf import PdfPages
import sys
import glob

def buscar_pasta_jogadores():
    """Tenta encontrar a pasta 'jogadores' no diretório atual ou em subdiretórios."""
    # Tentar no diretório atual
    if os.path.exists('jogadores'):
        return 'jogadores'
    
    # Tentar em um nível acima
    if os.path.exists('../jogadores'):
        return '../jogadores'
    
    # Buscar em subdiretórios
    for root, dirs, _ in os.walk('.'):
        if 'jogadores' in dirs:
            return os.path.join(root, 'jogadores')
    
    # Tentar encontrar pasta jogadores_bak
    if os.path.exists('jogadores_bak'):
        return 'jogadores_bak'
    
    if os.path.exists('../jogadores_bak'):
        return '../jogadores_bak'
    
    # Buscar arquivos de personagens em qualquer lugar
    arquivos_personagens = glob.glob('**/*_historico.txt', recursive=True)
    if arquivos_personagens:
        # Retorna o diretório do primeiro arquivo encontrado
        return os.path.dirname(arquivos_personagens[0])
    
    return None

def extrair_valores_de_arquivo(caminho_arquivo):
    """Extrai dados de um arquivo de estatísticas de personagem."""
    try:
        with open(caminho_arquivo, 'r', encoding='utf-8') as arquivo:
            conteudo = arquivo.read()
            
        # Extrair nome do personagem
        match_nome = re.search(r'Personagem: (\w+)', conteudo)
        nome = match_nome.group(1) if match_nome else "Desconhecido"
        
        # Extrair estatísticas da sessão atual
        media_sessao = float(re.search(r'Média de rolagem \(sessão atual\): ([\d.]+)', conteudo).group(1))
        desvio_padrao_sessao = float(re.search(r'Desvio padrão \(sessão atual\): ([\d.]+)', conteudo).group(1))
        variancia_sessao = float(re.search(r'Variância \(sessão atual\): ([\d.]+)', conteudo).group(1))
        
        # Extrair estatísticas cumulativas
        total_sessoes = int(re.search(r'Total de sessões: (\d+)', conteudo).group(1))
        total_rolagens = int(re.search(r'Total de rolagens \(todas as sessões\): (\d+)', conteudo).group(1))
        media_total = float(re.search(r'Média total \(todas as sessões\): ([\d.]+)', conteudo).group(1))
        desvio_padrao_total = float(re.search(r'Desvio padrão total: ([\d.]+)', conteudo).group(1))
        variancia_total = float(re.search(r'Variância total: ([\d.]+)', conteudo).group(1))
        
        # Extrair distribuição de rolagens da sessão atual
        distribuicao = {}
        for i in range(1, 21):
            match = re.search(rf'Valor {i}: ([\d.]+)%', conteudo)
            if match:
                distribuicao[i] = float(match.group(1))
            else:
                distribuicao[i] = 0.0
        
        # Extrair rolagens individuais
        rolagens = []
        matches = re.findall(r'Rolagem \d+: (\d+)', conteudo)
        for match in matches:
            rolagens.append(int(match))
            
        return {
            'nome': nome,
            'media_sessao': media_sessao,
            'desvio_padrao_sessao': desvio_padrao_sessao,
            'variancia_sessao': variancia_sessao,
            'total_sessoes': total_sessoes,
            'total_rolagens': total_rolagens,
            'media_total': media_total,
            'desvio_padrao_total': desvio_padrao_total,
            'variancia_total': variancia_total,
            'distribuicao': distribuicao,
            'rolagens': rolagens
        }
    except Exception as e:
        print(f"Erro ao processar o arquivo {caminho_arquivo}: {e}")
        return None

def extrair_historico(caminho_arquivo):
    """Extrai o histórico de rolagens de todas as sessões."""
    try:
        with open(caminho_arquivo, 'r', encoding='utf-8') as arquivo:
            conteudo = arquivo.read()
            
        historico = []
        sessao_atual = []
        sessao_num = 0
        
        for linha in conteudo.split('\n'):
            if 'Sessão' in linha:
                if sessao_atual:
                    historico.append({'sessao': sessao_num, 'rolagens': sessao_atual})
                sessao_atual = []
                sessao_num += 1
            elif 'Rolagem' in linha and ':' in linha:
                valor = int(linha.split(':')[-1].strip())
                sessao_atual.append(valor)
        
        # Adicionar a última sessão se houver dados
        if sessao_atual:
            historico.append({'sessao': sessao_num, 'rolagens': sessao_atual})
            
        return historico
    except Exception as e:
        print(f"Erro ao processar o histórico {caminho_arquivo}: {e}")
        return []

def criar_graficos(dados_personagens, pasta_saida='graficos'):
    """Cria gráficos baseados nos dados dos personagens."""
    # Criar pasta de saída se não existir
    if not os.path.exists(pasta_saida):
        os.makedirs(pasta_saida)
    
    # Configurações gerais
    sns.set_style("whitegrid")
    cores = sns.color_palette("husl", len(dados_personagens))
    
    # Criar PDF para todos os gráficos
    pdf_path = os.path.join(pasta_saida, 'relatorio_completo.pdf')
    with PdfPages(pdf_path) as pdf:
        # 1. Comparação de médias entre personagens
        plt.figure(figsize=(12, 6))
        nomes = [dados['nome'] for dados in dados_personagens]
        medias_sessao = [dados['media_sessao'] for dados in dados_personagens]
        medias_total = [dados['media_total'] for dados in dados_personagens]
        
        x = np.arange(len(nomes))
        largura = 0.35
        
        plt.bar(x - largura/2, medias_sessao, largura, label='Média da Sessão Atual', color='skyblue')
        plt.bar(x + largura/2, medias_total, largura, label='Média Total', color='navy')
        
        plt.axhline(y=10.5, color='red', linestyle='--', label='Média Ideal (10.5)')
        plt.ylabel('Média de Rolagem')
        plt.xlabel('Personagem')
        plt.title('Comparação de Médias de Rolagem por Personagem')
        plt.xticks(x, nomes)
        plt.legend()
        plt.tight_layout()
        plt.savefig(os.path.join(pasta_saida, 'comparacao_medias.png'))
        pdf.savefig()
        plt.close()
        
        # 2. Distribuição de rolagens por personagem
        for i, dados in enumerate(dados_personagens):
            plt.figure(figsize=(10, 6))
            valores = list(dados['distribuicao'].keys())
            frequencias = list(dados['distribuicao'].values())
            
            plt.bar(valores, frequencias, color=cores[i])
            plt.axhline(y=5, color='red', linestyle='--', label='Distribuição Ideal (5%)')
            plt.xlabel('Valor da Rolagem')
            plt.ylabel('Frequência (%)')
            plt.title(f'Distribuição de Rolagens - {dados["nome"]}')
            plt.xticks(valores)
            plt.legend()
            plt.tight_layout()
            plt.savefig(os.path.join(pasta_saida, f'distribuicao_{dados["nome"]}.png'))
            pdf.savefig()
            plt.close()
        
        # 3. Box plot das rolagens atuais
        plt.figure(figsize=(12, 6))
        dados_box = []
        for dados in dados_personagens:
            dados_box.append(dados['rolagens'])
        
        plt.boxplot(dados_box, labels=nomes)
        plt.axhline(y=10.5, color='red', linestyle='--', label='Média Ideal (10.5)')
        plt.ylabel('Valor da Rolagem')
        plt.title('Comparação da Distribuição de Rolagens (Sessão Atual)')
        plt.legend()
        plt.tight_layout()
        plt.savefig(os.path.join(pasta_saida, 'boxplot_rolagens.png'))
        pdf.savefig()
        plt.close()
        
        # 4. Desvio padrão comparativo
        plt.figure(figsize=(12, 6))
        desvios_sessao = [dados['desvio_padrao_sessao'] for dados in dados_personagens]
        desvios_total = [dados['desvio_padrao_total'] for dados in dados_personagens]
        
        plt.bar(x - largura/2, desvios_sessao, largura, label='Desvio Padrão (Sessão Atual)', color='lightgreen')
        plt.bar(x + largura/2, desvios_total, largura, label='Desvio Padrão (Total)', color='darkgreen')
        
        plt.axhline(y=5.766, color='red', linestyle='--', label='Desvio Padrão Ideal (5.766)')
        plt.ylabel('Desvio Padrão')
        plt.xlabel('Personagem')
        plt.title('Comparação de Desvio Padrão por Personagem')
        plt.xticks(x, nomes)
        plt.legend()
        plt.tight_layout()
        plt.savefig(os.path.join(pasta_saida, 'comparacao_desvios.png'))
        pdf.savefig()
        plt.close()
    
    print(f"Gráficos salvos na pasta '{pasta_saida}' e relatório completo em '{pdf_path}'")

def processar_historico_e_criar_graficos_tendencia(pasta_jogadores, pasta_saida='graficos'):
    """Processa os arquivos de histórico e cria gráficos de tendência ao longo das sessões."""
    if not os.path.exists(pasta_saida):
        os.makedirs(pasta_saida)
    
    nomes_personagens = ["Nicolle", "Jaeyk", "Thorne", "Riley", "Imugi", "Dean", "Mestre"]
    dados_historicos = {}
    
    # Processar histórico de cada personagem
    for nome in nomes_personagens:
        arquivo_historico = os.path.join(pasta_jogadores, f"{nome}_historico.txt")
        if os.path.exists(arquivo_historico):
            historico = extrair_historico(arquivo_historico)
            if historico:
                dados_historicos[nome] = historico
    
    if not dados_historicos:
        print("Nenhum arquivo de histórico encontrado.")
        return
    
    # Criar gráfico de tendência da média por sessão
    plt.figure(figsize=(14, 8))
    markers = ['o', 's', '^', 'D', 'v', '<', '>']
    
    for i, (nome, historico) in enumerate(dados_historicos.items()):
        sessoes = []
        medias = []
        
        for sessao in historico:
            sessoes.append(sessao['sessao'])
            medias.append(sum(sessao['rolagens']) / len(sessao['rolagens']) if sessao['rolagens'] else 0)
        
        plt.plot(sessoes, medias, marker=markers[i % len(markers)], label=nome)
    
    plt.axhline(y=10.5, color='red', linestyle='--', label='Média Ideal (10.5)')
    plt.xlabel('Número da Sessão')
    plt.ylabel('Média de Rolagem')
    plt.title('Tendência de Médias por Sessão')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(pasta_saida, 'tendencia_medias.png'))
    plt.close()
    
    # Criar gráfico de calor (heatmap) para visualizar todas as rolagens
    for nome, historico in dados_historicos.items():
        # Preparar dados para o heatmap
        todos_dados = []
        for sessao in historico:
            for valor in sessao['rolagens']:
                todos_dados.append({'Sessão': sessao['sessao'], 'Valor': valor})
        
        if todos_dados:
            df = pd.DataFrame(todos_dados)
            # Criar pivot table para contar ocorrências
            pivot = pd.crosstab(index=df['Sessão'], columns=df['Valor'])
            
            # Normalizar para percentuais por sessão
            for sessao in pivot.index:
                total = pivot.loc[sessao].sum()
                if total > 0:
                    pivot.loc[sessao] = (pivot.loc[sessao] / total) * 100
            
            # Garantir que todas as colunas de 1 a 20 existam
            for i in range(1, 21):
                if i not in pivot.columns:
                    pivot[i] = 0
            pivot = pivot.reindex(sorted(pivot.columns), axis=1)
            
            plt.figure(figsize=(12, 8))
            sns.heatmap(pivot, cmap="YlGnBu", annot=True, fmt=".1f", cbar_kws={'label': 'Frequência (%)'})
            plt.title(f'Distribuição de Rolagens por Sessão - {nome}')
            plt.ylabel('Número da Sessão')
            plt.xlabel('Valor Rolado')
            plt.tight_layout()
            plt.savefig(os.path.join(pasta_saida, f'heatmap_{nome}.png'))
            plt.close()
    
    print(f"Gráficos de tendência salvos na pasta '{pasta_saida}'")

def main():
    # Verificar se o caminho foi fornecido como argumento
    if len(sys.argv) > 1:
        pasta_jogadores = sys.argv[1]
    else:
        # Tentar encontrar automaticamente
        pasta_jogadores = buscar_pasta_jogadores()
        
        if not pasta_jogadores:
            print("Não foi possível encontrar a pasta 'jogadores' ou 'jogadores_bak'.")
            print("Por favor, especifique o caminho como argumento:")
            print("python visualizador_rpg.py caminho/para/pasta/jogadores")
            
            # Perguntar ao usuário o caminho
            pasta_jogadores = input("Digite o caminho para a pasta com os arquivos de personagens: ").strip()
            
            if not pasta_jogadores or not os.path.exists(pasta_jogadores):
                print(f"O caminho '{pasta_jogadores}' não existe ou é inválido.")
                return
    
    pasta_saida = 'graficos'  # Pasta onde serão salvos os gráficos
    
    # Verificar se a pasta existe
    if not os.path.exists(pasta_jogadores):
        print(f"A pasta '{pasta_jogadores}' não existe.")
        return
    
    print(f"Usando pasta de dados: {pasta_jogadores}")
    
    # Lista de personagens
    nomes_personagens = ["Nicolle", "Jaeyk", "Thorne", "Riley", "Imugi", "Dean", "Mestre"]
    dados_personagens = []
    
    # Processar cada arquivo de personagem
    arquivos_encontrados = False
    for nome in nomes_personagens:
        arquivo = os.path.join(pasta_jogadores, f"{nome}.txt")
        if os.path.exists(arquivo):
            arquivos_encontrados = True
            dados = extrair_valores_de_arquivo(arquivo)
            if dados:
                dados_personagens.append(dados)
    
    # Se não encontrar arquivos com os nomes predefinidos, tenta buscar qualquer arquivo .txt
    if not arquivos_encontrados:
        print("Arquivos de personagens predefinidos não encontrados. Buscando quaisquer arquivos de texto...")
        arquivos_txt = glob.glob(os.path.join(pasta_jogadores, "*.txt"))
        for arquivo in arquivos_txt:
            # Excluir arquivos de histórico
            if "_historico" not in arquivo:
                dados = extrair_valores_de_arquivo(arquivo)
                if dados:
                    dados_personagens.append(dados)
    
    if dados_personagens:
        criar_graficos(dados_personagens, pasta_saida)
        processar_historico_e_criar_graficos_tendencia(pasta_jogadores, pasta_saida)
        print(f"Processamento concluído! {len(dados_personagens)} personagens analisados.")
    else:
        print("Nenhum arquivo de personagem válido encontrado.")
        print(f"Verifique se os arquivos estão presentes em: {pasta_jogadores}")

if __name__ == "__main__":
    main()