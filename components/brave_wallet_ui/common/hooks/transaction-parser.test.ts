import { renderHook } from '@testing-library/react-hooks'

import { TransactionType } from '../../constants/types'
import {
  getMockedTransactionInfo,
  mockAccount,
  mockAssetPrices,
  mockERC20Token,
  mockNetwork
} from '../constants/mocks'
import { SwapExchangeProxy } from './address-labels'
import { useTransactionParser } from './transaction-parser'

describe('useTransactionParser hook', () => {
  describe('check for sameAddressError', () => {
    describe.each([
      ['ERC20Transfer', TransactionType.ERC20Transfer, 'recipient'],
      ['ERC20Approve', TransactionType.ERC20Approve, 'approval target']
    ])('%s', (_, txType, toLabel) => {
      it(`should be defined when sender and ${toLabel} are same`, () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txArgs: ['0xdeadbeef', 'foo']
        })

        expect(parsedTransaction.sameAddressError).toBeDefined()
      })

      it(`should be undefined when sender and ${toLabel} are different`, () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txArgs: ['0xbadcafe', 'foo']
        })

        expect(parsedTransaction.sameAddressError).toBeUndefined()
      })
    })

    describe.each([
      ['ERC721TransferFrom', TransactionType.ERC721TransferFrom],
      ['ERC721SafeTransferFrom', TransactionType.ERC721SafeTransferFrom]
    ])('%s', (_, txType) => {
      it('should be undefined when sender and recipient are same', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txArgs: ['mockOwner', '0xdeadbeef', 'mockTokenID']
        })

        expect(parsedTransaction.sameAddressError).toBeUndefined()
      })

      it('should be defined when owner and recipient are same', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: 'mockFromAddress',
          txArgs: ['0xdeadbeef', '0xdeadbeef', 'mockTokenID']
        })

        expect(parsedTransaction.sameAddressError).toBeDefined()
      })

      it('should be undefined when owner and recipient are different', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: 'mockFromAddress',
          txArgs: ['mockOwner', 'mockToAddress', 'mockTokenID']
        })

        expect(parsedTransaction.sameAddressError).toBeUndefined()
      })
    })

    describe.each([
      // ETHSend can have same sender and recipient in case of cancel transactions
      ['ETHSend', TransactionType.ETHSend],
      ['Other', TransactionType.Other],
      ['0x Swap', TransactionType.Other]
    ])('%s', (name, txType) => {
      it('should always be undefined', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txData: {
            ...mockTransactionInfo.txData,
            baseData: {
              ...mockTransactionInfo.txData.baseData,
              to: name === '0x Swap' ? SwapExchangeProxy : '0xdeadbeef'
            }
          }
        })

        expect(parsedTransaction.sameAddressError).toBeUndefined()
      })
    })
  })

  describe('check for contractAddressError', () => {
    describe.each([
      ['ERC20Approve', TransactionType.ERC20Approve],
      ['ETHSend', TransactionType.ETHSend],
      ['Other', TransactionType.Other],
      ['0x Swap', TransactionType.Other]
    ])('%s', (name, txType) => {
      it('should always be undefined', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [],
          [{ ...mockERC20Token, contractAddress: '0xdeadbeef' }]
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txArgs: txType === TransactionType.ETHSend ? [] : ['mockArg1', 'mockArg2'],
          txType,
          txData: {
            ...mockTransactionInfo.txData,
            baseData: {
              ...mockTransactionInfo.txData.baseData,
              to: name === '0x Swap' ? SwapExchangeProxy : '0xdeadbeef'
            }
          }
        })

        expect(parsedTransaction.contractAddressError).toBeUndefined()
      })
    })

    describe.each([
      ['ERC20Transfer', TransactionType.ERC20Transfer],
      ['ERC721TransferFrom', TransactionType.ERC721TransferFrom],
      ['ERC721SafeTransferFrom', TransactionType.ERC721SafeTransferFrom]
    ])('%s', (_, txType) => {
      it('should be defined when recipient is a known contract address', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [],
          [{ ...mockERC20Token, contractAddress: '0xdeadbeef' }]
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txArgs: txType === TransactionType.ERC20Transfer
            ? ['0xdeadbeef', 'mockAmount']
            : ['mockOwner', '0xdeadbeef', 'mockTokenID'],
          txType
        })

        expect(parsedTransaction.contractAddressError).toBeDefined()
      })

      it('should be undefined when recipient is an unknown contract address', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [],
          [{ ...mockERC20Token, contractAddress: '0xdeadbeef' }]
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txArgs: txType === TransactionType.ERC20Transfer
            ? ['0xbadcafe', 'mockAmount']
            : ['mockOwner', '0xbadcafe', 'mockTokenID'],
          txType
        })

        expect(parsedTransaction.contractAddressError).toBeUndefined()
      })
    })
  })

  describe('check for insufficientFundsError', () => {
    describe.each([
      ['ERC20Transfer', TransactionType.ERC20Transfer],
      ['ERC20Approve', TransactionType.ERC20Approve],
      ['ERC721TransferFrom', TransactionType.ERC721TransferFrom],
      ['ERC721SafeTransferFrom', TransactionType.ERC721SafeTransferFrom],
      ['ETHSend', TransactionType.ETHSend],
      ['Other', TransactionType.Other]
    ])('%s', (_, txType) => {
      it('should be true when funds are insufficient for gas', () => {
        /**
         * Account balance: 0.001 ETH
         * Transaction value: 0 ETH
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         */

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            tokens: [{
              asset: mockERC20Token,
              assetBalance: '0x0', // 0 DOG
              fiatBalance: '0'
            }],
            address: '0xdeadbeef',
            balance: '0x38d7ea4c68000' // 0.001 ETH
          }],
          mockAssetPrices, [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txArgs: [TransactionType.ERC20Approve, TransactionType.ERC20Transfer].includes(txType)
            ? ['mockRecipient', '0x0']
            : ['mockOwner', 'mockRecipient', 'mockTokenID'],
          txType,
          txData: {
            ...mockTransactionInfo.txData,
            baseData: {
              ...mockTransactionInfo.txData.baseData,
              to: txType === TransactionType.ERC20Transfer
                ? mockERC20Token.contractAddress
                : mockTransactionInfo.txData.baseData.to,
              value: '0x0', // 0 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00' // 150 Gwei
            }
          }
        })

        expect(parsedTransaction.insufficientFundsError).toBeTruthy()
      })

      it('should be false when funds are sufficient for gas', () => {
        /**
         * Account balance: 1 ETH
         * Transaction value: 0 ETH
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         */

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            address: '0xdeadbeef',
            balance: '0xde0b6b3a7640000' // 1 ETH
          }],
          mockAssetPrices, [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txArgs: [TransactionType.ERC20Approve, TransactionType.ERC20Transfer].includes(txType)
            ? ['mockRecipient', '0x0']
            : ['mockOwner', 'mockRecipient', 'mockTokenID'],
          txType,
          txData: {
            ...mockTransactionInfo.txData,
            baseData: {
              ...mockTransactionInfo.txData.baseData,
              value: '0x0',
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00' // 150 Gwei
            }
          }
        })

        expect(parsedTransaction.insufficientFundsError).toBeFalsy()
      })
    })

    describe.each([
      ['ETHSend', TransactionType.ETHSend],
      ['Other', TransactionType.Other]
    ])('%s', (_, txType) => {
      it('should be true when funds are insufficient for send amount', () => {
        /**
         * Account balance: 0.001 ETH
         * Transaction value: 1 ETH
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         *
         * Remarks: sufficient funds for gas, but not for send amount.
         */

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            address: '0xdeadbeef',
            balance: '0x38d7ea4c68000' // 0.001 ETH
          }],
          mockAssetPrices, [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txType,
          txData: {
            ...mockTransactionInfo.txData,
            baseData: {
              ...mockTransactionInfo.txData.baseData,
              value: '0xde0b6b3a7640000', // 1 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00' // 150 Gwei
            }
          }
        })

        expect(parsedTransaction.insufficientFundsError).toBeTruthy()
      })

      it('should be false when funds are sufficient for send amount', () => {
        /**
         * Account balance: 1.00315 ETH
         * Transaction value: 1 ETH
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         *
         * Remarks: sufficient funds for gas, and for send amount.
         */

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            address: '0xdeadbeef',
            balance: '0xdebe79c2e6ee000' // 1.00315 ETH
          }],
          mockAssetPrices, [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txType,
          txData: {
            ...mockTransactionInfo.txData,
            baseData: {
              ...mockTransactionInfo.txData.baseData,
              value: '0xde0b6b3a7640000', // 1 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00' // 150 Gwei
            }
          }
        })

        expect(parsedTransaction.insufficientFundsError).toBeFalsy()
      })
    })

    describe('ERC20Transfer', () => {
      it('should be true when funds are insufficient for send amount', () => {
        /**
         * Account ETH balance: 0.00315 ETH
         * Account DOG balance: 0.001 DOG
         * Transaction value: 1 DOG
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         *
         * Remarks: sufficient funds for gas, but not for send amount.
         */

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            tokens: [{
              asset: mockERC20Token,
              assetBalance: '0x38d7ea4c68000', // 0.001 DOG
              fiatBalance: '0'
            }],
            address: '0xdeadbeef',
            balance: '0xb30e8870ae000' // 0.00315 ETH
          }],
          mockAssetPrices, [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txArgs: [
            'mockRecipient',
            '0xde0b6b3a7640000' // 1 DOG
          ],
          txType: TransactionType.ERC20Transfer,
          txData: {
            ...mockTransactionInfo.txData,
            baseData: {
              ...mockTransactionInfo.txData.baseData,
              to: mockERC20Token.contractAddress,
              value: '0x0', // 0 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00' // 150 Gwei
            }
          }
        })

        expect(parsedTransaction.insufficientFundsError).toBeTruthy()
      })

      it('should be false when funds are sufficient for send amount', () => {
        /**
         * Account ETH balance: 0.00315 ETH
         * Account DOG balance: 1 DOG
         * Transaction value: 1 DOG
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         *
         * Remarks: sufficient funds for gas, but not for send amount.
         */

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            tokens: [{
              asset: mockERC20Token,
              assetBalance: '0xde0b6b3a7640000', // 1 DOG
              fiatBalance: '0'
            }],
            address: '0xdeadbeef',
            balance: '0xb30e8870ae000' // 0.00315 ETH
          }],
          mockAssetPrices, [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txArgs: [
            'mockRecipient',
            '0xde0b6b3a7640000' // 1 DOG
          ],
          txType: TransactionType.ERC20Transfer,
          txData: {
            ...mockTransactionInfo.txData,
            baseData: {
              ...mockTransactionInfo.txData.baseData,
              to: mockERC20Token.contractAddress,
              value: '0x0', // 0 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00' // 150 Gwei
            }
          }
        })

        expect(parsedTransaction.insufficientFundsError).toBeFalsy()
      })
    })
  })
})