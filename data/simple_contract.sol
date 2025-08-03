// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

/**
 * @title SimpleContract
 * @dev 一个简单的智能合约示例，演示基本的状态管理和计算功能
 * @author chord233
 */
contract SimpleContract {
    // 状态变量
    uint256 public value;
    address public owner;
    mapping(address => uint256) public balances;
    
    // 事件定义
    event ValueChanged(uint256 oldValue, uint256 newValue);
    event BalanceUpdated(address indexed user, uint256 newBalance);
    
    // 修饰符
    modifier onlyOwner() {
        require(msg.sender == owner, "Only owner can call this function");
        _;
    }
    
    modifier validValue(uint256 _value) {
        require(_value > 0, "Value must be greater than 0");
        _;
    }
    
    /**
     * @dev 构造函数
     * @param _initialValue 初始值
     */
    constructor(uint256 _initialValue) {
        value = _initialValue;
        owner = msg.sender;
        emit ValueChanged(0, _initialValue);
    }
    
    /**
     * @dev 设置新的值
     * @param _newValue 新值
     */
    function setValue(uint256 _newValue) public onlyOwner validValue(_newValue) {
        uint256 oldValue = value;
        value = _newValue;
        emit ValueChanged(oldValue, _newValue);
    }
    
    /**
     * @dev 获取当前值
     * @return 当前存储的值
     */
    function getValue() public view returns (uint256) {
        return value;
    }
    
    /**
     * @dev 增加值
     * @param _amount 要增加的数量
     */
    function addValue(uint256 _amount) public validValue(_amount) {
        uint256 oldValue = value;
        value += _amount;
        emit ValueChanged(oldValue, value);
    }
    
    /**
     * @dev 计算两个数的和
     * @param a 第一个数
     * @param b 第二个数
     * @return 两数之和
     */
    function add(uint256 a, uint256 b) public pure returns (uint256) {
        return a + b;
    }
    
    /**
     * @dev 计算两个数的乘积
     * @param a 第一个数
     * @param b 第二个数
     * @return 两数之积
     */
    function multiply(uint256 a, uint256 b) public pure returns (uint256) {
        return a * b;
    }
    
    /**
     * @dev 更新用户余额
     * @param _user 用户地址
     * @param _balance 新余额
     */
    function updateBalance(address _user, uint256 _balance) public onlyOwner {
        balances[_user] = _balance;
        emit BalanceUpdated(_user, _balance);
    }
    
    /**
     * @dev 获取用户余额
     * @param _user 用户地址
     * @return 用户余额
     */
    function getBalance(address _user) public view returns (uint256) {
        return balances[_user];
    }
    
    /**
     * @dev 转移所有权
     * @param _newOwner 新所有者地址
     */
    function transferOwnership(address _newOwner) public onlyOwner {
        require(_newOwner != address(0), "New owner cannot be zero address");
        owner = _newOwner;
    }
    
    /**
     * @dev 计算阶乘（递归实现）
     * @param n 输入数字
     * @return 阶乘结果
     */
    function factorial(uint256 n) public pure returns (uint256) {
        if (n <= 1) {
            return 1;
        }
        return n * factorial(n - 1);
    }
    
    /**
     * @dev 检查数字是否为偶数
     * @param n 输入数字
     * @return 如果是偶数返回true，否则返回false
     */
    function isEven(uint256 n) public pure returns (bool) {
        return n % 2 == 0;
    }
    
    /**
     * @dev 获取合约信息
     * @return 合约所有者、当前值和合约余额
     */
    function getContractInfo() public view returns (address, uint256, uint256) {
        return (owner, value, address(this).balance);
    }
    
    /**
     * @dev 接收以太币
     */
    receive() external payable {
        // 合约可以接收以太币
    }
    
    /**
     * @dev 回退函数
     */
    fallback() external payable {
        // 处理未知函数调用
    }
}