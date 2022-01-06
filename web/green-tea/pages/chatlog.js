import Head from "next/head";
import ChatContainer from "../components/ChatContainer";
import Sidebar from "../components/Sidebar";
import Session from "./session";

function chatlog() {
    let page = (
        <div className="flex">
            <Head>
                <title>GreenTea Dashboard</title>
                <link rel="icon" href="/greentea.ico" />
            </Head>
            <Sidebar />
            <ChatContainer />
        </div>
    );

    return <Session page={page} />;
}

export default chatlog;
