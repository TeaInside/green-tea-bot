import Head from "next/head";
import Sidebar from "../components/Sidebar";
import ChatContainer from "../components/ChatContainer";

function chatlog() {
    return (
        <div className="flex">
            <Head>
                <title>GreenTea Dashboard</title>
                <link rel="icon" href="/greentea.ico" />
            </Head>
            <Sidebar />
            <ChatContainer/>
        </div>
    );
}

export default chatlog;
