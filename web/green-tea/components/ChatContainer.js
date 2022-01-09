import React from "react";
import CONFIG from "../config.json";
import ChatBox from "./ChatBox";
import GroupList from "./GroupList";

class ChatContainer extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            loading: true,
            loadingChatBox: false,
            error: false,
            groupName: null,
            chatListdata: [],
            chatBoxData: [],
            chatBoxDataCache: {},
            lastMessageLoaded: false,
            fetchPtr: 0,
            activeChatId: 0,
        };
    }

    async fetchChatListdata() {
        let url = CONFIG.BASE_API_URL + "/api.php?action=get_group_list";
        let data = await fetch(url).then((res) => res.json());
        this.setState({
            loading: false,
            chatListdata: data.msg.data,
        });
    }

    async fetchChatData(chat_id, limit = 300, callback = null) {
        let reactThis = this;
        let url = CONFIG.BASE_API_URL + "/api.php?action=get_chat_messages&group_id=" + chat_id + "&limit=" + limit;
        await fetch(url).then(async function (res) {
            let data = await res.json();
            reactThis.state.chatBoxDataCache[chat_id] = data.msg.data;
            if (callback) callback();
        });
        this.setState({ lastMessageLoaded: true });
    }

    getLastMessage(chat_id) {
        if (!(chat_id in this.state.chatBoxDataCache)) return "";

        let msgList = this.state.chatBoxDataCache[chat_id];
        if (msgList.length == 0) return "";

        let msg = msgList[msgList.length - 1];

        return msg.first_name + ": " + msg.text;
    }

    async fetchChatBoxData(chat_id, group_name, limit = 300) {
        if (chat_id in this.state.chatBoxDataCache) {
            this.setState({
                loadingChatBox: false,
                groupName: group_name,
                activeChatId: chat_id,
                chatBoxData: this.state.chatBoxDataCache[chat_id],
            });
            return;
        }

        this.setState({ loadingChatBox: true });
        await this.fetchChatData(chat_id, limit);
        this.setState({
            loadingChatBox: false,
            groupName: group_name,
            activeChatId: chat_id,
            chatBoxData: this.state.chatBoxDataCache[chat_id],
        });
    }

    async componentDidMount() {
        await this.fetchChatListdata();
        let chatListData = this.state.chatListdata;
        let reactThis = this;
        let callback = function () {
            if (reactThis.state.fetchPtr < chatListData.length) reactThis.fetchChatData(chatListData[reactThis.state.fetchPtr++].tg_group_id, 10, callback);
        };
        callback();
    }

    render() {
        if (this.state.error)
            return (
                <div>
                    <h1>Fetch error {this.state.error.toString()}</h1>
                </div>
            );

        if (this.state.loading)
            return (
                <div className="w-full">
                    <h1>Loading Data...</h1>
                </div>
            );

        let chatBoxElement;
        if (this.state.loadingChatBox) {
            chatBoxElement = (
                <div className="w-full">
                    <h1>Loading Data...</h1>
                </div>
            );
        } else {
            if (this.state.groupName === null) {
                chatBoxElement = (
                    <div className="w-full text-center h-screen items-center justify-center">
                        <h1 className="text-[2em] text-gray-600 text-opacity-50 font-semibold mt-10">Welcome to Log Chat</h1>
                        <img className="w-4/12 mx-auto mt-8" src="./greentea.svg" alt="GreenTea Icon" />
                    </div>
                );
            } else {
                chatBoxElement = <ChatBox container={this} data={this.state.chatBoxData} groupName={this.state.groupName} />;
            }
        }

        return (
            <div className="flex-grow flex w-full h-screen bg-cream">
                <GroupList container={this} list={this.state.chatListdata} />
                {chatBoxElement}
            </div>
        );
    }
}

export default ChatContainer;
