// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

/**
 * @title VotingContract
 * @dev 一个去中心化投票系统智能合约
 * @author chord233
 */
contract VotingContract {
    // 投票选项结构
    struct Proposal {
        uint256 id;
        string name;
        string description;
        uint256 voteCount;
        bool active;
        uint256 createdAt;
        uint256 deadline;
    }
    
    // 投票者结构
    struct Voter {
        bool registered;
        bool hasVoted;
        uint256 votedProposal;
        uint256 registeredAt;
    }
    
    // 状态变量
    address public administrator;
    uint256 public proposalCount;
    uint256 public voterCount;
    uint256 public votingDuration = 7 days; // 默认投票持续时间
    bool public votingActive;
    
    // 映射
    mapping(uint256 => Proposal) public proposals;
    mapping(address => Voter) public voters;
    mapping(uint256 => address[]) public proposalVoters; // 每个提案的投票者列表
    
    // 事件
    event ProposalCreated(uint256 indexed proposalId, string name, uint256 deadline);
    event VoterRegistered(address indexed voter);
    event VoteCast(address indexed voter, uint256 indexed proposalId);
    event VotingStarted();
    event VotingEnded();
    event WinnerDeclared(uint256 indexed proposalId, string name, uint256 voteCount);
    
    // 修饰符
    modifier onlyAdministrator() {
        require(msg.sender == administrator, "Only administrator can perform this action");
        _;
    }
    
    modifier onlyRegisteredVoter() {
        require(voters[msg.sender].registered, "You must be registered to vote");
        _;
    }
    
    modifier votingIsActive() {
        require(votingActive, "Voting is not currently active");
        _;
    }
    
    modifier hasNotVoted() {
        require(!voters[msg.sender].hasVoted, "You have already voted");
        _;
    }
    
    modifier validProposal(uint256 _proposalId) {
        require(_proposalId > 0 && _proposalId <= proposalCount, "Invalid proposal ID");
        require(proposals[_proposalId].active, "Proposal is not active");
        _;
    }
    
    /**
     * @dev 构造函数
     */
    constructor() {
        administrator = msg.sender;
        votingActive = false;
        proposalCount = 0;
        voterCount = 0;
    }
    
    /**
     * @dev 创建新的投票提案
     * @param _name 提案名称
     * @param _description 提案描述
     */
    function createProposal(string memory _name, string memory _description) 
        public onlyAdministrator {
        require(bytes(_name).length > 0, "Proposal name cannot be empty");
        require(bytes(_description).length > 0, "Proposal description cannot be empty");
        
        proposalCount++;
        uint256 deadline = block.timestamp + votingDuration;
        
        proposals[proposalCount] = Proposal({
            id: proposalCount,
            name: _name,
            description: _description,
            voteCount: 0,
            active: true,
            createdAt: block.timestamp,
            deadline: deadline
        });
        
        emit ProposalCreated(proposalCount, _name, deadline);
    }
    
    /**
     * @dev 注册投票者
     * @param _voter 投票者地址
     */
    function registerVoter(address _voter) public onlyAdministrator {
        require(_voter != address(0), "Invalid voter address");
        require(!voters[_voter].registered, "Voter already registered");
        
        voters[_voter] = Voter({
            registered: true,
            hasVoted: false,
            votedProposal: 0,
            registeredAt: block.timestamp
        });
        
        voterCount++;
        emit VoterRegistered(_voter);
    }
    
    /**
     * @dev 批量注册投票者
     * @param _voters 投票者地址数组
     */
    function registerVoters(address[] memory _voters) public onlyAdministrator {
        for (uint256 i = 0; i < _voters.length; i++) {
            if (_voters[i] != address(0) && !voters[_voters[i]].registered) {
                voters[_voters[i]] = Voter({
                    registered: true,
                    hasVoted: false,
                    votedProposal: 0,
                    registeredAt: block.timestamp
                });
                voterCount++;
                emit VoterRegistered(_voters[i]);
            }
        }
    }
    
    /**
     * @dev 开始投票
     */
    function startVoting() public onlyAdministrator {
        require(!votingActive, "Voting is already active");
        require(proposalCount > 0, "No proposals available");
        
        votingActive = true;
        emit VotingStarted();
    }
    
    /**
     * @dev 结束投票
     */
    function endVoting() public onlyAdministrator {
        require(votingActive, "Voting is not active");
        
        votingActive = false;
        emit VotingEnded();
    }
    
    /**
     * @dev 投票
     * @param _proposalId 提案ID
     */
    function vote(uint256 _proposalId) 
        public 
        onlyRegisteredVoter 
        votingIsActive 
        hasNotVoted 
        validProposal(_proposalId) {
        
        require(block.timestamp <= proposals[_proposalId].deadline, "Voting deadline has passed");
        
        // 记录投票
        voters[msg.sender].hasVoted = true;
        voters[msg.sender].votedProposal = _proposalId;
        
        // 增加提案票数
        proposals[_proposalId].voteCount++;
        
        // 记录投票者
        proposalVoters[_proposalId].push(msg.sender);
        
        emit VoteCast(msg.sender, _proposalId);
    }
    
    /**
     * @dev 获取获胜提案
     * @return 获胜提案的ID、名称和票数
     */
    function getWinner() public view returns (uint256, string memory, uint256) {
        require(proposalCount > 0, "No proposals available");
        
        uint256 winningVoteCount = 0;
        uint256 winningProposal = 0;
        
        for (uint256 i = 1; i <= proposalCount; i++) {
            if (proposals[i].voteCount > winningVoteCount) {
                winningVoteCount = proposals[i].voteCount;
                winningProposal = i;
            }
        }
        
        require(winningProposal > 0, "No winner found");
        
        return (winningProposal, proposals[winningProposal].name, winningVoteCount);
    }
    
    /**
     * @dev 宣布获胜者
     */
    function declareWinner() public onlyAdministrator {
        require(!votingActive, "Voting must be ended first");
        
        (uint256 winningProposal, string memory winnerName, uint256 voteCount) = getWinner();
        emit WinnerDeclared(winningProposal, winnerName, voteCount);
    }
    
    /**
     * @dev 获取提案详情
     * @param _proposalId 提案ID
     * @return 提案的所有信息
     */
    function getProposal(uint256 _proposalId) 
        public view validProposal(_proposalId) 
        returns (uint256, string memory, string memory, uint256, bool, uint256, uint256) {
        
        Proposal memory proposal = proposals[_proposalId];
        return (
            proposal.id,
            proposal.name,
            proposal.description,
            proposal.voteCount,
            proposal.active,
            proposal.createdAt,
            proposal.deadline
        );
    }
    
    /**
     * @dev 获取所有提案的投票结果
     * @return 提案ID数组和对应的票数数组
     */
    function getAllResults() public view returns (uint256[] memory, uint256[] memory) {
        uint256[] memory proposalIds = new uint256[](proposalCount);
        uint256[] memory voteCounts = new uint256[](proposalCount);
        
        for (uint256 i = 1; i <= proposalCount; i++) {
            proposalIds[i-1] = i;
            voteCounts[i-1] = proposals[i].voteCount;
        }
        
        return (proposalIds, voteCounts);
    }
    
    /**
     * @dev 获取投票者信息
     * @param _voter 投票者地址
     * @return 投票者的注册状态、投票状态和投票的提案ID
     */
    function getVoterInfo(address _voter) 
        public view returns (bool, bool, uint256, uint256) {
        
        Voter memory voter = voters[_voter];
        return (voter.registered, voter.hasVoted, voter.votedProposal, voter.registeredAt);
    }
    
    /**
     * @dev 获取提案的投票者列表
     * @param _proposalId 提案ID
     * @return 投票者地址数组
     */
    function getProposalVoters(uint256 _proposalId) 
        public view validProposal(_proposalId) returns (address[] memory) {
        return proposalVoters[_proposalId];
    }
    
    /**
     * @dev 设置投票持续时间
     * @param _duration 持续时间（秒）
     */
    function setVotingDuration(uint256 _duration) public onlyAdministrator {
        require(_duration > 0, "Duration must be greater than 0");
        votingDuration = _duration;
    }
    
    /**
     * @dev 停用提案
     * @param _proposalId 提案ID
     */
    function deactivateProposal(uint256 _proposalId) 
        public onlyAdministrator validProposal(_proposalId) {
        proposals[_proposalId].active = false;
    }
    
    /**
     * @dev 获取合约统计信息
     * @return 提案总数、投票者总数、投票状态
     */
    function getContractStats() public view returns (uint256, uint256, bool) {
        return (proposalCount, voterCount, votingActive);
    }
    
    /**
     * @dev 转移管理员权限
     * @param _newAdmin 新管理员地址
     */
    function transferAdministration(address _newAdmin) public onlyAdministrator {
        require(_newAdmin != address(0), "New admin cannot be zero address");
        administrator = _newAdmin;
    }
}