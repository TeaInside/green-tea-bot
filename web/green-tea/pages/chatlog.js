import ChatBox from "../components/ChatBox";
import GroupList from "../components/GroupList";
import Sidebar from "../components/Sidebar";

function chatlog() {
    return (
        <div className="flex">
            <Sidebar />
            <GroupList />
            <ChatBox />
        </div>
    );
}

export default chatlog;
