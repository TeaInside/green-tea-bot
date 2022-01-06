import React from "react";
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
        };
    }

    async fetchChatListdata() {
        let url = "https://greentea-api.teainside.org/api.php?action=get_group_list";
        let data = await fetch(url).then((res) => res.json());
        this.setState({
            loading: false,
            chatListdata: data.msg.data,
        });
    }

    async fetchChatBoxData(chat_id, group_name, limit = 10) {
        if (chat_id in this.state.chatBoxDataCache) {
            this.setState({
                loadingChatBox: false,
                groupName: group_name,
                chatBoxData: this.state.chatBoxDataCache[chat_id],
            });
            return;
        }

        this.setState({ loadingChatBox: true });
        let url = "https://greentea-api.teainside.org/api.php?action=get_chat_messages&group_id=" + chat_id + "&limit=" + limit;
        let data = await fetch(url).then((res) => res.json());
        this.state.chatBoxDataCache[chat_id] = data.msg.data;
        this.setState({
            loadingChatBox: false,
            groupName: group_name,
            chatBoxData: this.state.chatBoxDataCache[chat_id],
        });
    }

    async componentDidMount() {
        await this.fetchChatListdata();
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
                <div>
                    <h1>Loading Data...</h1>
                </div>
            );

        let chatBoxElement;
        if (this.state.loadingChatBox) {
            chatBoxElement = (
                <div>
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
                chatBoxElement = <ChatBox data={this.state.chatBoxData} groupName={this.state.groupName} />;
            }
        }

        return (
            <div className="flex-grow flex h-screen bg-cream">
                <GroupList container={this} list={this.state.chatListdata} />
                {chatBoxElement}
            </div>
        );
    }
}

export default ChatContainer;
