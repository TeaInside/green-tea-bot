import Head from "next/head";
import ChatBox from "../components/ChatBox";
import GroupList from "../components/GroupList";
import Sidebar from "../components/Sidebar";

function chatlog({ groupData, messageGroup }) {
    return (
        <div className="flex">
            <Head>
                <title>GreenTea Dashboard</title>
                <link rel="icon" href="/greentea.ico" />
            </Head>
            <Sidebar />
            <GroupList list={groupData.msg.data} />
            <ChatBox data={messageGroup.msg.data} />
        </div>
    );
}

export default chatlog;

export async function getServerSideProps(context) {
    // const products = await fetch("https://fakestoreapi.com/products").then((res) => res.json());
    const groupData = await fetch("https://www.teainside.org/fp123/get_group_list.json").then((res) => res.json());
    const messageGroup = await fetch("https://www.teainside.org/fp123/get_group_messages.json").then((res) => res.json());
    // const data = await res.json();

    return {
        props: {
            groupData,
            messageGroup,
        },
    };
}
