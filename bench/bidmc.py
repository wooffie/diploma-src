import pandas as pd

if __name__ == '__main__':
    # Manual breath annotations
    breaths = pd.read_csv('BIDMC/bidmc_01_Breaths.csv')
    # Fixed variables
    fix = pd.read_csv('BIDMC/bidmc_01_Fix.txt')
    # Parameters
    numerics = pd.read_csv('BIDMC/bidmc_01_Numerics.csv')
    # Signals
    signals = pd.read_csv('BIDMC/bidmc_01_Signals.csv')

    # Не подходит, нет красного и ИК светодиодов
