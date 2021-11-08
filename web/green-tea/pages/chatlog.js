import Head from "next/head";
import ChatBox from "../components/ChatBox";
import GroupList from "../components/GroupList";
import Sidebar from "../components/Sidebar";

function chatlog() {
    return (
        <div className="flex">
            <Head>
                <title>GreenTea Dashboard</title>
                <link rel="icon" href="/greentea.ico" />
            </Head>
            <Sidebar />
            <GroupList />
            <ChatBox />
        </div>
    );
}

export default chatlog;
